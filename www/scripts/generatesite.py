import os
import string
import shutil

searchfor = ['$TSVNSHORTVERSION$', '$TSVNVERSION$', '$SVNVERSION$']
replacewith = ['1.8.4', '1.8.4.24972', '1.8.5']
setpath = '/var/www/domains/tortoisesvn.net/htdocs'

for root, dirs, files in os.walk(setpath, topdown=False):
    for name in files:
        os.remove(os.path.join(root, name))
    for name in dirs:
        os.rmdir(os.path.join(root, name))
if os.path.exists(setpath):
    os.rmdir(setpath)
shutil.copytree(os.path.abspath('..'), setpath)

footerFile = file(os.path.join(setpath, 'templates/footer.html'), 'r')
footerJSFile = file(os.path.join(setpath, 'templates/footer-js.html'), 'r')
menuFile = file(os.path.join(setpath, 'templates/menu.html'), 'r')
menuFileDE = file(os.path.join(setpath, 'templates/menu.de.html'), 'r')
menuFileZH = file(os.path.join(setpath, 'templates/menu.zh.html'), 'r')
sidebarFile = file(os.path.join(setpath, 'templates/sidebar.html'), 'r')
sidebarFileDE = file(os.path.join(setpath, 'templates/sidebar.de.html'), 'r')
sidebarFileZH = file(os.path.join(setpath, 'templates/sidebar.zh.html'), 'r')

footerText = footerFile.read()
footerFile.close()

footerJSText = footerJSFile.read()
footerJSFile.close()

menuText = menuFile.read()
menuFile.close()

menuTextDE = menuFileDE.read()
menuFileDE.close()

menuTextZH = menuFileZH.read()
menuFileZH.close()

sidebarText = sidebarFile.read()
sidebarFile.close()

sidebarTextDE = sidebarFileDE.read()
sidebarFileDE.close()

sidebarTextZH = sidebarFileZH.read()
sidebarFileZH.close()


searchfor.extend(['<!--#include FILE="menu.html"-->',
                  '<!--#include FILE="menu.de.html"-->',
                  '<!--#include FILE="menu.zh.html"-->',
                  '<!--#include FILE="sidebar.html"-->',
                  '<!--#include FILE="sidebar.de.html"-->',
                  '<!--#include FILE="sidebar.zh.html"-->',
                  '<!--#include FILE="footer.html"-->',
                  '<!--#include FILE="footer-js.html"-->'])
replacewith.extend([menuText,
                    menuTextDE,
                    menuTextZH,
                    sidebarText,
                    sidebarTextDE,
                    sidebarTextZH,
                    footerText,
                    footerJSText])

for root, dirs, files in os.walk(setpath):
    fname = files
    for fname in files:
        if fname.endswith('.html'):
            inputFile = file(os.path.join(root, fname), 'r')
            data = inputFile.read()
            inputFile.close()
            for i in range(len(searchfor)):
                x = searchfor[i]
                y = replacewith[i]
                search = string.find(data, x)
                if search >= 1:
                    data = data.replace(x, y)

            outputFile = file(os.path.join(root, fname), 'w')
            outputFile.write(data)
            outputFile.close()
