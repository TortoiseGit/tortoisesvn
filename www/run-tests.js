#!/usr/bin/env node

/**!
 * run-tests.js, script to run csslint and JSHint for our files
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

require('shelljs/global');

cd(__dirname);

//
// JSHint
//
JSHINT_BIN = './node_modules/jshint/bin/jshint';

if (!test('-f', JSHINT_BIN)) {
    echo('JSHint not found. Run `npm install` in the root dir first.');
    exit(1);
}

if (exec('node' +' ' + JSHINT_BIN +' ' + 'make.js run-tests.js').code !== 0) {
    echo('*** JSHint failed! (return code != 0)');
    echo();
} else {
    echo('JSHint completed successfully');
    echo();
}


//
// csslint
//
CSSLINT_BIN = './node_modules/csslint/cli.js';

if (!test('-f', CSSLINT_BIN)) {
    echo('csslint not found. Run `npm install` in the root dir first.');
    exit(1);
}

if (exec('node' +' ' + CSSLINT_BIN +' ' + 'css/style.css').code !== 0) {
    echo('*** csslint failed! (return code != 0)');
    echo();
}
