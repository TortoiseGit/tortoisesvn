/* jshint browser:true */
/* global baguetteBox */

(function() {
    'use strict';

    var selector = '.entry';
    var el = document.querySelectorAll(selector);

    if (el.length) {
        baguetteBox.run(selector, {
            async: false,
            buttons: true,
            noScrollbars: true
        });
    }

})();
