<?php

class mysql {
}

include("mysql_user.php");

$db=mysql_connect($mysqlhost, $mysqluser, $mysqlpass);

if ($db===false) {
	die(mysql_error());
}

mysql_select_db($mysqldb, $db);

php?>