// Source: https://www.feedthebot.com/pagespeed/defer-images.html

/* jshint browser: true */

(function() {
    'use strict';

    function loadOnLoad() {

        var iframeEl = document.createElement('iframe');
        iframeEl.setAttribute('src', 'https://www.openhub.net/p/tortoisesvn/widgets/project_users?style=blue');
        iframeEl.scrolling = 'no';
        iframeEl.frameborder = '0';
        iframeEl.style.border = 'none';
        iframeEl.style.width = '95px';
        iframeEl.style.height = '115px';
        document.getElementById('iframe-badge').appendChild(iframeEl);

        var imgDefer = document.getElementsByTagName('img');
        for (var i = 0; i < imgDefer.length; i++) {
            if (imgDefer[i].getAttribute('data-src')) {
                imgDefer[i].setAttribute('src', imgDefer[i].getAttribute('data-src'));
            }
        }

    }

    if (window.addEventListener) {
        window.addEventListener('load', loadOnLoad, false);
    } else if (window.attachEvent) {
        window.attachEvent('onload', loadOnLoad);
    } else {
        window.onload = loadOnLoad;
    }

}());
