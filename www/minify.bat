@echo off
rem install node.js and then run:
rem npm install -g clean-css
rem npm install -g uglify-js

pushd %~dp0

type css\jquery.fancybox.css css\normalize.css css\style.css | cleancss --s0 -o css\pack.css
cmd /c cleancss --s0 -o css\prettify.min.css css\prettify.css
cmd /c uglifyjs js\jquery.fancybox.js js\jquery.mousewheel.js --compress --mangle -o js\pack.js

popd
