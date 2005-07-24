<html>
<head>
<title>Subversion Repositories</title>
</head>
<body>

<h2>Subversion Repositories</h2>
<p>
<?php
    $svnparentpath = "C:/svn";
    $svnparenturl = "/svn";

    $dh = opendir( $svnparentpath );
    if( $dh ) {
        while( $dir = readdir( $dh ) ) {
            $svndir = $svnparentpath . "/" . $dir;
            $svndbdir = $svndir . "/db";
            $svnfstypefile = $svndbdir . "/fs-type";
            if( is_dir( $svndir ) && is_dir( $svndbdir ) ) 
						{
              echo "<a href=\"" . $svnparenturl . "/" . $dir . "\">" .
                        $dir . "</a>\n";
  						if( file_exists( $svnfstypefile ) )
              {
              	$handle = fopen ("$svnfstypefile", "r");
  							$buffer = fgets($handle, 4096);
  							fclose( $handle );
  							$buffer = chop( $buffer );
  							if( strcmp( $buffer, "fsfs" )==0 )
  							{
  								echo " (FSFS) <br />\n";
  							}
  							else
  							{
  								echo " (BDB) <br />\n";
  							}
              }
              else
              {
              	echo " (BDB) <br />\n";
              }
            }
        }
        closedir( $dh );
    }
?>
</p>

</body>
</html> 
