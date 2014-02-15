/**!
 * make.js, script to build the website for TortoiseSVN
 *
 * Copyright (C) 2013 - TortoiseSVN
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

(function () {
    "use strict";

    require("shelljs/make");
    var fs = require("fs"),
        CleanCSS = require("clean-css"),
        UglifyJS = require("uglify-js"),
        ROOT_DIR = __dirname + "/";     // absolute path to project's root

    //
    // make minify
    //
    target.minify = function () {
        cd(ROOT_DIR);
        echo();
        echo("### Minifying css files...");

        // prettify.min.css
        var inCss1 = cat("css/prettify.css");

        var minifier1 = new CleanCSS({
                keepSpecialComments: 1,
                compatibility: "ie8"
            });

        fs.writeFileSync("css/prettify.min.css", minifier1.minify(inCss1), "utf8");

        echo();
        echo("### Finished css/prettify.min.css.");

        // pack.css

        var inCss2 = cat(["css/normalize.css",
                         "css/jquery.fancybox.css",
                         "css/style.css"
        ]);

        var minifier2 = new CleanCSS({
                keepSpecialComments: 0,
                compatibility: "ie8"
            });

        fs.writeFileSync("css/pack.css", minifier2.minify(inCss2), "utf8");

        echo();
        echo("### Finished css/pack.css.");

        echo();
        echo("### Minifying js files...");

        var inJs = cat(["js/jquery.fancybox.js",
                        "js/jquery.mousewheel.js"]);

        var minifiedJs = UglifyJS.minify(inJs, {
            compress: true,
            fromString: true, // this is needed to pass JS source code instead of filenames
            mangle: true,
            warnings: false
        });

        fs.writeFileSync("js/pack.js", minifiedJs.code, "utf8");

        echo();
        echo("### Finished js/pack.js.");
    };


    //
    // make all
    //
    target.all = function () {
        target.minify();
    };

    //
    // make help
    //
    target.help = function () {
        echo("Available targets:");
        echo("  minify  Creates the minified CSS and JS");
        echo("  help    shows this help message");
    };

}());
