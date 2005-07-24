/*
 * console.c: various interactive-prompt routines shared between
 * the Windows console PuTTY tools
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "putty.h"
#include "storage.h"
#include "ssh.h"
#include "LoginDialog.h"

int console_batch_mode = FALSE;

static void *console_logctx = NULL;

/*
 * Clean up and exit.
 */
void cleanup_exit(int code)
{
    /*
     * Clean up.
     */
    sk_cleanup();

    random_save_seed();
#ifdef MSCRYPTOAPI
    crypto_wrapup();
#endif

    exit(code);
}

void verify_ssh_host_key(void *frontend, char *host, int port, char *keytype,
			 char *keystr, char *fingerprint)
{
    int ret;

    static const char absentmsg[] =
	"The server's host key is not cached in the registry. You\n"
	"have no guarantee that the server is the computer you\n"
	"think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"If you trust this host, hit Yes to add the key to\n"
	"%s's cache and carry on connecting.\n"
	"If you want to carry on connecting just once, without\n"
	"adding the key to the cache, hit No.\n"
	"If you do not trust this host, hit Cancel to abandon the\n"
	"connection.\n";

    static const char wrongmsg[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"\n"
	"The server's host key does not match the one %s has\n"
	"cached in the registry. This means that either the\n"
	"server administrator has changed the host key, or you\n"
	"have actually connected to another computer pretending\n"
	"to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"If you were expecting this change and trust the new key,\n"
	"hit Yes to update %s's cache and continue connecting.\n"
	"If you want to carry on connecting but without updating\n"
	"the cache, hit No.\n"
	"If you want to abandon the connection completely, hit\n"
	"Cancel. Hitting Cancel is the ONLY guaranteed safe\n" "choice.\n";

    static const char mbtitle[] = "%s Security Alert";

    /*
     * Verify the key against the registry.
     */
    ret = verify_host_key(host, port, keytype, keystr);

    if (ret == 0)		       /* success - key matched OK */
	return;
    if (ret == 2) {		       /* key was different */
	int mbret;
	char *message, *title;
	message = dupprintf(wrongmsg, appname, keytype, fingerprint, appname);
	title = dupprintf(mbtitle, appname);
	mbret = MessageBox(GetParentHwnd(), message, title,
			   MB_ICONWARNING | MB_YESNOCANCEL);
	sfree(message);
	sfree(title);
	if (mbret == IDYES)
	    store_host_key(host, port, keytype, keystr);
	if (mbret == IDCANCEL)
	    cleanup_exit(0);
    }
    if (ret == 1) {		       /* key was absent */
	int mbret;
	char *message, *title;
	message = dupprintf(absentmsg, keytype, fingerprint, appname);
	title = dupprintf(mbtitle, appname);
	mbret = MessageBox(GetParentHwnd(), message, title,
			   MB_ICONWARNING | MB_YESNOCANCEL);
	sfree(message);
	sfree(title);
	if (mbret == IDYES)
	    store_host_key(host, port, keytype, keystr);
	if (mbret == IDCANCEL)
	    cleanup_exit(0);
    }
}

void update_specials_menu(void *frontend)
{
}

/*
 * Ask whether the selected cipher is acceptable (since it was
 * below the configured 'warn' threshold).
 * cs: 0 = both ways, 1 = client->server, 2 = server->client
 */
void askcipher(void *frontend, char *ciphername, int cs)
{
    static const char mbtitle[] = "%s Security Alert";
    static const char msg[] =
	"The first %.35scipher supported by the server\n"
	"is %.64s, which is below the configured\n"
	"warning threshold.\n"
	"Do you want to continue with this connection?\n";
    char *message, *title;
    int mbret;

    message = dupprintf(msg, ((cs == 0) ? "" :
			      (cs == 1) ? "client-to-server " :
			      "server-to-client "), ciphername);
    title = dupprintf(mbtitle, appname);
    mbret = MessageBox(GetParentHwnd(), message, title,
		       MB_ICONWARNING | MB_YESNO);
    sfree(message);
    sfree(title);
    if (mbret == IDYES)
	return;
    else
	cleanup_exit(0);
}

/*
 * Ask whether to wipe a session log file before writing to it.
 * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
 */
int askappend(void *frontend, Filename filename)
{
    static const char msgtemplate[] =
	"The session log file \"%.*s\" already exists.\n"
	"You can overwrite it with a new session log,\n"
	"append your session log to the end of it,\n"
	"or disable session logging for this session.\n"
	"Hit Yes to wipe the file, No to append to it,\n"
	"or Cancel to disable logging.";
    char *message;
    char *mbtitle;
    int mbret;

    message = dupprintf(msgtemplate, FILENAME_MAX, filename.path);
    mbtitle = dupprintf("%s Log to File", appname);

    mbret = MessageBox(GetParentHwnd(), message, mbtitle,
		       MB_ICONQUESTION | MB_YESNOCANCEL);

    sfree(message);
    sfree(mbtitle);

    if (mbret == IDYES)
	return 2;
    else if (mbret == IDNO)
	return 1;
    else
	return 0;
}

/*
 * Warn about the obsolescent key file format.
 * 
 * Uniquely among these functions, this one does _not_ expect a
 * frontend handle. This means that if PuTTY is ported to a
 * platform which requires frontend handles, this function will be
 * an anomaly. Fortunately, the problem it addresses will not have
 * been present on that platform, so it can plausibly be
 * implemented as an empty function.
 */
void old_keyfile_warning(void)
{
    static const char mbtitle[] = "%s Key File Warning";
    static const char message[] =
	"You are loading an SSH 2 private key which has an\n"
	"old version of the file format. This means your key\n"
	"file is not fully tamperproof. Future versions of\n"
	"%s may stop supporting this private key format,\n"
	"so we recommend you convert your key to the new\n"
	"format.\n"
	"\n"
	"You can perform this conversion by loading the key\n"
	"into PuTTYgen and then saving it again.";

    char *msg, *title;
    msg = dupprintf(message, appname);
    title = dupprintf(mbtitle, appname);

    MessageBox(GetParentHwnd(), msg, title, MB_OK);

    sfree(msg);
    sfree(title);
}

void console_provide_logctx(void *logctx)
{
    console_logctx = logctx;
}

void logevent(void *frontend, const char *string)
{
    if (console_logctx)
	log_eventlog(console_logctx, string);
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

void frontend_keypress(void *handle)
{
    /*
     * This is nothing but a stub, in console code.
     */
    return;
}
