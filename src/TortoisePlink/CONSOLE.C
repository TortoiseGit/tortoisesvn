/*
 * console.c: various interactive-prompt routines shared between
 * the console PuTTY tools
 */

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "putty.h"
#include "storage.h"
#include "ssh.h"

#include "LoginDialog.h"

int console_batch_mode = FALSE;

/*
 * Clean up and exit.
 */
void cleanup_exit(int code)
{
    /*
     * Clean up.
     */
    sk_cleanup();
    WSACleanup();

    if (cfg.protocol == PROT_SSH) {
   random_save_seed();
#ifdef MSCRYPTOAPI
   crypto_wrapup();
#endif
    }

    exit(code);
}

void verify_ssh_host_key(char *host, int port, char *keytype,
          char *keystr, char *fingerprint)
{
    int ret;
    static const char absentmsg_batch[] =
   "The server's host key is not cached in the registry. You\n"
   "have no guarantee that the server is the computer you\n"
   "think it is.\n"
   "The server's key fingerprint is:\n"
   "%s\n"
   "Connection abandoned.\n";
    static const char absentmsg[] =
   "The server's host key is not cached in the registry. You\n"
   "have no guarantee that the server is the computer you\n"
   "think it is.\n"
   "The server's key fingerprint is:\n"
   "%s\n"
   "If you trust this host, hit Yes to add the key to\n"
   "PuTTY's cache and carry on connecting.\n"
   "If you want to carry on connecting just once, without\n"
   "adding the key to the cache, hit No.\n"
   "If you do not trust this host, hit Cancel to abandon the\n"
   "connection.";

    static const char wrongmsg_batch[] =
   "WARNING - POTENTIAL SECURITY BREACH!\n"
   "The server's host key does not match the one PuTTY has\n"
   "cached in the registry. This means that either the\n"
   "server administrator has changed the host key, or you\n"
   "have actually connected to another computer pretending\n"
   "to be the server.\n"
   "The new key fingerprint is:\n"
   "%s\n"
   "Connection abandoned.\n";
    static const char wrongmsg[] =
   "WARNING - POTENTIAL SECURITY BREACH!\n"
   "The server's host key does not match the one PuTTY has\n"
   "cached in the registry. This means that either the\n"
   "server administrator has changed the host key, or you\n"
   "have actually connected to another computer pretending\n"
   "to be the server.\n"
   "The new key fingerprint is:\n"
   "%s\n"
   "If you were expecting this change and trust the new key,\n"
   "hit Yes to update PuTTY's cache and continue connecting.\n"
   "If you want to carry on connecting but without updating\n"
   "the cache, hit No.\n"
   "If you want to abandon the connection completely, hit\n"
   "Cancel. Cancel is the ONLY guaranteed safe choice.";

    static const char mbtitle[] = "TortoisePlink Security Alert";
    
    char message[160 +
                 /* sensible fingerprint max size */
                 (sizeof(absentmsg) > sizeof(wrongmsg) ?
                  sizeof(absentmsg) : sizeof(wrongmsg))];

    /*
     * Verify the key against the registry.
     */
    ret = verify_host_key(host, port, keytype, keystr);

    if (ret == 0)           /* success - key matched OK */
       return;

    if (ret == 2) {            /* key was different */
         int mbret;
         sprintf(message, wrongmsg, fingerprint);
         mbret = MessageBox(GetParentHwnd(), message, mbtitle,
                            MB_ICONWARNING | MB_YESNOCANCEL);
         if (mbret == IDYES)
             store_host_key(host, port, keytype, keystr);
         if (mbret == IDCANCEL)
             exit(0);
    }

   if (ret == 1) {             /* key was absent */
      int mbret;
      sprintf(message, absentmsg, fingerprint);
      mbret = MessageBox(GetParentHwnd(), message, mbtitle,
                         MB_ICONWARNING | MB_YESNOCANCEL);
      if (mbret == IDYES)
         store_host_key(host, port, keytype, keystr);
      if (mbret == IDCANCEL)
         cleanup_exit(0);
   }
}

/*
 * Ask whether the selected cipher is acceptable (since it was
 * below the configured 'warn' threshold).
 * cs: 0 = both ways, 1 = client->server, 2 = server->client
 */
void askcipher(char *ciphername, int cs)
{
   static const char mbtitle[] = "TortoisePlink Security Alert";
   
   static const char msg[] =
   "The first %.35scipher supported by the server\n"
   "is %.64s, which is below the configured\n"
   "warning threshold.\n"
   "Do you want to continue with this connection?";

   /* guessed cipher name + type max length */
   char message[100 + sizeof(msg)];
   int mbret;
  
   sprintf(message, msg,
           (cs == 0) ? "" :
           (cs == 1) ? "client-to-server " : "server-to-client ",
           ciphername);
  
   mbret = MessageBox(GetParentHwnd(), message, mbtitle,
                      MB_ICONWARNING | MB_YESNO);
   if (mbret == IDYES)
      return;
   else
      cleanup_exit(0);
}

/*
 * Ask whether to wipe a session log file before writing to it.
 * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
 */
int askappend(char *filename)
{
    static const char mbtitle[] = "TortoisePlink Log to File";
  
    static const char msgtemplate[] =
    "The session log file \"%.*s\" already exists.\n"
    "You can overwrite it with a new session log,\n"
    "append your session log to the end of it,\n"
    "or disable session logging for this session.\n"
    "Hit Yes to wipe the file, No to append to it,\n"
    "or Cancel to disable logging.";
    char message[sizeof(msgtemplate) + FILENAME_MAX];
    int mbret;
    if (cfg.logxfovr != LGXF_ASK) {
       return ((cfg.logxfovr == LGXF_OVR) ? 2 : 1);
    }
    sprintf(message, msgtemplate, FILENAME_MAX, filename);
 
    mbret = MessageBox(GetParentHwnd(), message, mbtitle,
                       MB_ICONQUESTION | MB_YESNOCANCEL);
    if (mbret == IDYES)
       return 2;
    else if (mbret == IDNO)
       return 1;
    else
       return 0;
}

/*
 * Warn about the obsolescent key file format.
 */
void old_keyfile_warning(void)
{
   static const char mbtitle[] = "TortoisePlink Key File Warning";
    static const char message[] =
   "You are loading an SSH 2 private key which has an\n"
   "old version of the file format. This means your key\n"
   "file is not fully tamperproof. Future versions of\n"
   "PuTTY may stop supporting this private key format,\n"
   "so we recommend you convert your key to the new\n"
   "format.\n"
   "\n"
   "Once the key is loaded into PuTTYgen, you can perform\n"
   "this conversion simply by saving it again.\n";

   MessageBox(GetParentHwnd(), message, mbtitle, MB_OK);
}

void logevent(char *string)
{
}

int console_get_line(const char *prompt, char *str,
             int maxlen, int is_pw)
{
   if (console_batch_mode) {
      if (maxlen > 0)
         str[0] = '\0';
   } else {
      if (DoLoginDialog(str, maxlen -1, prompt, is_pw))
         return 1;
      else
         cleanup_exit(1);
   }
   return 1;
}
