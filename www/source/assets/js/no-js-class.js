/* jshint browser: true */

(function() {
    'use strict';

    var docEl = document.documentElement;
    var classRE = new RegExp('(^|\\s)no-js(\\s|$)');
    var className = docEl.className;

    docEl.className = className.replace(classRE, '$1js$2');
})();
