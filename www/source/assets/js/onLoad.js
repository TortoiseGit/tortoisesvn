/* jshint browser: true */

(function() {
    'use strict';

    function initOnLoad() {
        // Original source: https://varvy.com/pagespeed/defer-images.html
        var imgElement = document.getElementsByTagName('img');

        for (var i = 0, len = imgElement.length; i < len; i++) {
            if (imgElement[i].hasAttribute('data-src')) {
                imgElement[i].setAttribute('src', imgElement[i].getAttribute('data-src'));
                imgElement[i].removeAttribute('data-src');
                imgElement[i].className += ' loaded';
            }
        }
    }

    if (window.addEventListener) {
        window.addEventListener('load', initOnLoad, false);
    } else if (window.attachEvent) {
        window.attachEvent('onload', initOnLoad);
    } else {
        window.onload = initOnLoad;
    }

}());
