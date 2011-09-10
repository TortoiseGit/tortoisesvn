<?php // $Id$
define("MODULE_PATH", "../modules/");
define("EXT_PATH", MODULE_PATH."ext/");
define("LIB_PATH", MODULE_PATH."lib/");

#include("../scripts/session.php");
#include("mysql.php");

// common modules
include_once (EXT_PATH."Html.php");
#include_once (EXT_PATH."tools.php");
include_once (EXT_PATH."Table.php");
#include_once (MODULE_PATH."csv.php");
require_once (LIB_PATH."parsecsv/parsecsv.lib.php");

// local modules
include_once MODULE_PATH."po.php";
#include_once MODULE_PATH."rc.php";


#open html
$html = new Html;
$html->SetTitle("TortoiseSVN | The coolest Interface to (Sub)Version Control - Translation check");
$html->AddCssFile("/css/tsvn.css", "all");
$html->AddCssFile("/css/print.css", "print");
$html->AddMetaHttpEquiv("content-type", "text/html; charset=utf-8");
$html->AddMeta("keywords", "");
$html->AddMeta("description", "");
echo $html->openBody();

#log
include "log.hp";


#load intro texts
include MODULE_PATH."intro_0.php";
include MODULE_PATH."intro_1.php";
include MODULE_PATH."intro_2.php";
include MODULE_PATH."intro_3.php";

#eliminate crawlers
if ($crawler) {
	echo '<div><center><hr/>Icons by: <a href="http://dryicons.com" class="link">DryIcons</a></center></div>';
	exit;
}

//include MODULE_PATH."intro_4.php";
//include MODULE_PATH."intro_5.php";
//include MODULE_PATH."intro_6.php";
//include MODULE_PATH."intro_7.php";
//include MODULE_PATH."intro_8.php";
//include MODULE_PATH."intro_9.php";

# get and fix parameters
$lang=$_GET["l"]; // language
if ($lang=="") {
	$lang="";
}

$stable=$_GET["stable"]; // stable(branch) vs. trunk
if ($stable) {
	$stable=1;
} else {
	$stable=0;
}

$tx=$_GET["tx"]; // transifex vs. svn
if ($tx) {
	$tx=1;
} else {
	$tx=0;
}

$gx=$_GET["gx"]; // graph x axis type
switch ($gx) {
 case "date":
	break;
 default:
	$gx="rev";
}

$m=$_GET["m"]; // module
switch ($m) {
 case 'g':
 case 's':
 case 'm':
	break;
 case 'd':
	$m='s';
	break;
 default:
	$m='g';
}



# determine mode
include MODULE_PATH."trunk.php";



#load mode module



echo "ok".__LINE__;
flush();
