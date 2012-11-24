@echo off
rem install node.js and then run:
rem npm install -g clean-css
rem npm install -g uglify-js

pushd %~dp0

type css\lightbox.css css\normalize.css css\style.css | cleancss --s0 -o css\pack.css
cmd /c cleancss --s0 -o css\prettify.min.css css\prettify.css
cmd /c uglifyjs js\lightbox.js --compress --mangle -o js\lightbox.min.js

popd
