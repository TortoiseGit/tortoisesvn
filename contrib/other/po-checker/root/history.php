<?php

include_once("mysql.php");
//include_once("../modules/ext/table.php");
include_once("history.fncs.php");

$title="TortoiseSVN | The coolest Interface to (Sub)Version Control - Translation history";

$imWidth=1040;
$imHeight=220;

if (!isset($historyTest)) {
    $lang=$_GET["l"];
    if ($lang=="") {
        $lang="";
    }

    $group=$_GET["g"];
    if ($group=="") {
        $group="g";
    }
}
if (!preg_match("/^[a-z][a-z](_[A-Z][A-Z]|_[a-z][a-z][a-z])?$/", $lang)) {
    $lang="";
}
if (!preg_match("/[gdm]/", $group)) {
    $group="g";
}



$sql="SELECT min(`revision`) as mRev, max(`revision`) as MRev, min(`tot`) as mTot, max(`tot`) as MTot FROM state WHERE `language`='$lang' && `group`='$group'";
$res=mysql_query($sql, $db);
if ($res===false) {
    echo "<br><i>$query</i> <b>".mysql_error()."</b>";
    die ("In progress ?");
}
$table=new Table;
$table->importMysqlResult($res);
#$table->Output();
$minRev=$table->data[0][0]+0;
$maxRev=$table->data[0][1]+0;
$minTot=$table->data[0][2]+0;
$maxTot=$table->data[0][3]+0;

$sql="SELECT min(`revision`) as mRev, max(`revision`) as MRev, min(`tot`) as mTot, max(`tot`) as MTot FROM state WHERE `language`='$lang'";
$sql="SELECT min(`revision`) as mRev, max(`revision`) as MRev, min(`tot`) as mTot, max(`tot`) as MTot FROM state";
$res=mysql_query($sql, $db);
if ($res===false) {
    echo "<br><i>$query</i> <b>".mysql_error()."</b>";
    die ("In progress ?");
}
$table=new Table;
$table->importMysqlResult($res);
#$table->Output();
$minRev=$table->data[0][0]+0;
$maxRev=$table->data[0][1]+0;


// fix Rev range
if (!is_int($maxRev)||$maxRev==0) {
    $maxRev=1;
    $imWidth=10;
    $imHeight=10;
}
if ($maxRev-1000>$minRev) {
    $minRev=$maxRev-1000;
}
$minRev=0;
if (!is_int($maxTot)||$maxTot==0) {
    $maxTot=1;
}

if (isset($historyTest)) {
    $res=($maxTot>1);
    return $res;
}

$sql="SELECT * FROM state JOIN revisions ON state.revision=revisions.revision WHERE `state`.`language`='$lang' && `state`.`group`='$group' && `state`.`revision`>=$minRev";
#$sql="SELECT * FROM state WHERE `language`='$lang' && `group`='$group'";
$res=mysql_query($sql, $db);
if ($res===false) {
    echo "<br><i>$query</i> <b>".mysql_error()."</b>";
    die ("In progress ?");
}

$table=new Table;
$table->importMysqlResult($res);
//$table->Output();


$im=imagecreate($imWidth, $imHeight);
$color_background=imagecolorallocate($im, 250, 200, 230);
$background2_color=imagecolorallocate($im, 55, 100, 33);
$line_color=imagecolorallocate($im, 70, 120, 40);
$text_color=imagecolorallocate($im, 40, 75, 20);
$color_transparent=imagecolorallocatealpha($im, 40, 75, 20, 127);

$color_black=imagecolorallocate($im, 0, 0, 0);
$color_gray=imagecolorallocatealpha($im, 0, 0, 0, 100);
$color_red=imagecolorallocate($im, 250, 0, 0);
$color_blue=imagecolorallocate($im, 0, 0, 250);
$color_green=imagecolorallocate($im, 0, 250, 0);
$color_pink=imagecolorallocate($im, 250, 150, 250);

$color_empty=imagecolorallocate($im, 100, 125, 50);
$color_player=imagecolorallocate($im, 180, 0, 0);
$color_ally=imagecolorallocate($im, 255, 255, 0);

$font="/srv/www/php/tcpdf/fonts/arial.ttf";

imagefilledrectangle($im, 0, 0, $imWidth, $imHeight, $color_transparent);
imagettftext($im, 10, 0, $imWidth/2-480/2, 10, $text_color, $font, $title);
imagettftext($im, 10, 0, 0, 10, $text_color, $font, $lang);


//$minRev=$maxRev-5000;
$xzoom=1/1.03;
$yzoom=1/7;
//$xzoom=$imWidth/($maxRev*1.03);
$yzoom=($imHeight-30)/($maxTot*1.03);
$xfactor=$imWidth*$xzoom;


if ($group=="g") {
    $graphs=array(
        array(3, $color_black),
        array(4, $color_blue),
        array(9, $color_pink),
        array(8, $color_red));
} else {
    $graphs=array(
        array(3, $color_black),
        array(9, $color_pink),
        array(8, $color_red));
}

foreach ($table->data as &$record) {
    $unt=$record[8];
    $fuz=$record[9];

    $record[9]=$fuz+$unt;
}
unset($record);



$ip=$_SERVER["REMOTE_ADDR"];
$debug=false && preg_match("/217\.75\.82\.141/", $ip);


if (true) { // graph mode is revision
    for ($i=0; $i<$maxRev+999; $i+=1000) {
        $x=ConvertRevisionToRange($i, $minRev, $maxRev)*$xfactor;
        imageline($im, $x, 25, $x, $imHeight, $color_gray);
        imagettftext($im, 10, 0, $x, 22, $text_color, $font, "r".($i/1000).($i?"k":""));
    }
    for ($i=0; $i<($imHeight-30)/$yzoom; $i+=100) {
        $y=$imHeight-$i*$yzoom-1;
        imageline($im, 0, $y, $imWidth, $y, $color_gray);
//      imagettftext($im, 10, $i*$yzoom, $imWid, $text_color, $font, $i);
    }
    foreach ($graphs as $graph) {
        $index=$graph[0];
        $color=$graph[1];
        // calculate first point
        $record=$table->data[0];
        $revision=$record[10];
        $val=$record[$index];
        $x=ConvertRevisionToRange($revision, $minRev, $maxRev)*$xfactor;
        $y=$imHeight-$val*$yzoom-1;
        foreach ($table->data as $record) {
            $revision=$record[10];
            $val=$record[$index];
            $newX=ConvertRevisionToRange($revision, $minRev, $maxRev)*$xfactor;
            $newY=$imHeight-$val*$yzoom-1;
            imageline($im, $x, $y, $newX, $y, $color);
            imageline($im, $newX, $y, $newX, $newY, $color);
            $x=$newX;
            $y=$newY;
        }
        imageline($im, $x, $y, $x, $y, $color);
    }
} else if (true) { // graph mode is date
    for ($i=0; $i<$maxRev+999; $i+=1000) {
        $x=ConvertRevisionToRange($i, $minRev, $maxRev)*$xfactor;
        imageline($im, $x, 25, $x, $imHeight, $color_gray);
        imagettftext($im, 10, 0, $x, 22, $text_color, $font, "r".($i/1000).($i?"k":""));
    }
    for ($i=0; $i<($imHeight-30)/$yzoom; $i+=100) {
        $y=$imHeight-$i*$yzoom-1;
        imageline($im, 0, $y, $imWidth, $y, $color_gray);
//      imagettftext($im, 10, $i*$yzoom, $imWid, $text_color, $font, $i);
    }
    foreach ($graphs as $graph) {
        $index=$graph[0];
        $color=$graph[1];
        // calculate first point
        $record=$table->data[0];
        $revision=$record[10];
        $val=$record[$index];
        $x=ConvertRevisionToRange($revision, $minRev, $maxRev)*$xfactor;
        $y=$imHeight-$val*$yzoom-1;
        foreach ($table->data as $record) {
            $revision=$record[10];
            $val=$record[$index];
            $newX=ConvertRevisionToRange($revision, $minRev, $maxRev)*$xfactor;
            $newY=$imHeight-$val*$yzoom-1;
            imageline($im, $x, $y, $newX, $y, $color);
            imageline($im, $newX, $y, $newX, $newY, $color);
            $x=$newX;
            $y=$newY;
        }
        imageline($im, $x, $y, $x, $y, $color);
    }
}
if (!$debug) {
    header("Content-type: image/png");
    imagepng($im);
}

//php?>