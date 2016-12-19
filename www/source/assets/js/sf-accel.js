/* jshint browser:true */

(function() {
    'use strict';

    var link = document.querySelectorAll('.content a[href^="https://downloads.sourceforge.net/tortoisesvn/"]');
    var linksNum = link.length;

    if (linksNum === 0) {
        return;
    }

    (function() {
        var script = document.createElement('script');
        script.src = '//sourceforge.net/accelerator/js?partner_id=61';
        script.defer = true;
        var s = document.getElementsByTagName('script')[0];
        s.parentNode.insertBefore(script, s);

        // Add custom class to our download links
        for (var i = 0; i < linksNum; i++) {
            var className = 'sourceforge_accelerator_link';

            if (link[i].classList) {
                link[i].classList.add(className);
            } else {
                link[i].className += ' ' + className;   // This can be removed for IE > 10
            }
        }
    })();
})();
