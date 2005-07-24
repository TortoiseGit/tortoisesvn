svn_index.php
=============
A PHP script which lists all repositories below the SVNParentPath directive.
To make use of this script, copy it into your webroot folder. Then edit your
Apache config file:
- activate the module 'rewrite_module' (mod_rewrite.so).
- Add the following lines below your Subversion <Location> block:
    RewriteEngine on
    RewriteRule ^/svn$ /svn_index.php [PT]
    RewriteRule ^/svn/$ /svn_index.php [PT]
    RewriteRule ^/svn/index.html$ /svn_index.php [PT] 

