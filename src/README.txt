
               TortoiseSVN, a windows shell integration for Subversion
               =======================================================


Contents:

     I. A FEW POINTERS
    II. PARTICIPATING
   III. COMPILING
    IV. QUICKSTART GUIDE


I.    A FEW POINTERS

      This code is still under development.  For an overview, visit

         http://www.tortoisesvn.org/

      If you've never heard of Subversion visit

         http://subversion.tigris.org/


II.   PARTICIPATING

      If you want to help us developing TortoiseSVN be welcome!
      But you don't have to be a programmer to help - we also
      need people who design icons and logos, maintain the website
      or just report bugs.


      A. Sourcecode

      The source code is documented with Doxygen comments. For more
      information on Doxygen visit http://www.doxygen.org. That means
      you can generate the source documentation yourself. At least
      the API TortoiseSVN uses is documented that way.
      We will provide a windows help file (*.chm) in the download
      section, but be aware that it won't be up to date all the time.

      The source is separated into two main modules: TortoiseShell and
      TortoiseProc. TortoiseShell is the shell integration part which
      provides the icon overlays, context menus and the column provider.
      TortoiseProc contains all the functions for subversion. Those
      functions are called from TortoiseShell via command line parameters.
      That way the shell won't block while the commands are executed and
      it also reduces the risk of harmful crashes (if TortoiseShell ever
      crashes it would take down the windows explorer also which could
      make a reboot necessary).

      B. External modules

      To compile TortoiseSVN you first must compile Subversion itself.
      See the Subversion documentation about that at
      http://subversion.tigris.org/.
      The directory structure is as follows:
      Subversion
        ac-helpers
        ...
        subversion
          bindings
          clients
          ...
          svnadmin
          svnlook
          tests
        ...
      TortoiseSVN
        src
          TortoiseProc
          TortoiseShell


III.  COMPILING

      The project has project and workspace files for MS Visual Studio .NET
      Sorry, there are no workspace or project files for VS6 at the moment.
      Even if there were, TortoiseSVN won't compile with it 'cause we use
      some of the new functions in MFC. But if someone want's to make
      the code compatible with VS6 be welcome to do so.
      Also, the crashhandler needs the dbghelp.dll which ships with
      WindowsXP, earlier versions don't work! So you have to get that
      dll from the microsoft website.

