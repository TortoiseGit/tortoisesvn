// Original source: https://varvy.com/pagespeed/defer-images.html

/* jshint browser: true */

(function() {
    'use strict';

    function initOnLoad() {

        var iframeElement = document.createElement('iframe');
        iframeElement.setAttribute('src', 'https://www.openhub.net/p/tortoisesvn/widgets/project_users?style=blue');
        iframeElement.scrolling = 'no';
        iframeElement.frameborder = '0';
        iframeElement.style.border = 'none';
        iframeElement.style.width = '95px';
        iframeElement.style.height = '115px';
        document.getElementById('iframe-badge').appendChild(iframeElement);

        var imgElement = document.getElementsByTagName('img');

        for (var i = 0; i < imgElement.length; i++) {
            if (imgElement[i].hasAttribute('data-src')) {
                imgElement[i].setAttribute('src', imgElement[i].getAttribute('data-src'));
                imgElement[i].removeAttribute('data-src');
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
