/**
 * \page Crasher
 * \section introduction Introduction
 * Have you ever had an application crashing on a customers machine
 * and have only "it just crashes" as a message from the customer?
 * Ever wish you could put some kind of "watchdog" that would monitor
 * your application on the customers computer and then if it crashed,
 * automatically scoop up where the crash happened, and allow the customer
 * to mail this information to you? If so then Crasher may be just what
 * you're looking for!
 * \nThe Crasher is a windows exception handler compiled as a dll which
 * provides the user with usefull information about the crash and the
 * system state at the time of the crash. The user can then either
 * just save this information for future use or directly mail it
 * to the developer of the application for helping in the development process.
 * \nCrasher displays a variety of information:
 * - the exception reason
 * - the registers
 * - a complete stacktrace from where the error occured
 * - information about the machine such as OS version, CPU, physical memory
 * - a list of all active processes and their loaded modules
 * \section usage Usage
 * To use the Crasher you first have to customize some messages and most
 * important the email-address the error files are sent to. You can do this
 * in the resource string table. Please make sure that you customize at least
 * the first three string table entries called IDS_APPNAME, IDS_MAILADDRESS
 * and IDS_ERRORLABEL, and also make sure that you do that in \b all languages!!!
 * \nAfter you edited the language table entries you have to compile the
 * Crasher project. Then in your main application you have to load that
 * dll:
 * \code
 * HINSTANCE hLib = LoadLibrary( "Crasher.dll" );
 * \endcode
 * and when your application exits you have to unload the module again:
 * \code
 * if (NULL != hLib)
 * {
 *	FreeLibrary(hLib);
 * }
 * \endcode
 * an easier way is to use an easy wrapper class:
 * \code
 * //CrashWrapper.h
 *
 * #pragma once
 *
 * class CCrashWrapper
 * {
 * public:
 * 	CCrashWrapper(void);
 * 	~CCrashWrapper(void);
 * private:
 * 	HINSTANCE	m_hLib;
 * };
 * \endcode
 *
 * \code
 * //CrashWrapper.cpp
 *
 * #include "StdAfx.h"
 * #include "crashhandler.h"
 *
 * CCrashHandler::CCrashHandler(void)
 * {
 * 	m_hLib = LoadLibrary( "Crasher.dll" );
 * }
 *
 * CCrashHandler::~CCrashHandler(void)
 * {
 * 	if ( NULL != m_hLib )
 * 	{
 * 		FreeLibrary( m_hLib );
 * 	}
 * }
 * \endcode
 * Just add the files CrashWrapper.h and CrashWrapper.cpp to your
 * project and then define one single object of that class at
 * global scope.
 *
 * \section deploying Deploying the application
 * For the Crasher to work you will need to ship the dbghelp.dll with
 * your application. But you \b must make sure that you ship at
 * least the version 5.1.2600.0 of that dll (which comes with
 * WindowsXP) or the Crasher won't work properly on earlier systems.
 * Place that dll in your application folder when installing.
 * The newest version of this dll is available at the microsoft website
 * at http://www.microsoft.com/ddk/debugging/default.asp. It's part of
 * the debugging tools package.
 * Microsoft states that this dll is redistributable so you won't
 * break the law by shipping it with your application.
 *
 * \section credits Credits
 * Most of the code was done by Jim Crafton in his BlackBox project which
 * is part of the VCF Framework. See http://sourceforge.net/projects/vcf
 * for information on that project. Also get a look at his article on
 * CodeProject named "Trapping Bugs with BlackBox" at
 * http://www.codeproject.com/script/Articles/list_articles.asp?userid=12433
 * \nI modified and enhanced that project to fit my needs.
 * The list of modifications I made so far:
 * - added MAPI support to send the report files via email
 * - removed network support 'cause you'll need a special server
 *   to receive reports sent that way (and I don't have such a server)
 * - added generating the MiniDump file which can be used in Visual Studio
 *   to "post mortem" debug the crashed application
 * - added internationalization support
 * - changed the about dialog box
 * - renamed the project to Crasher
 * - renamed some variables
 *
 */