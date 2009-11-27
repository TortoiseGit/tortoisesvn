<?php
include("../modules/ext/common.php");
include("../modules/ext/table.php");

$dir="/srv/www/sites/tsvn.e-posta.sk/data/";
include("mysql.php");
include("../modules/po.php");

//$query="REPLACE INTO state (`revision`, `group`, `language`, `translated`) VALUES($currentRev, 't', '-', $total)";
$sql = 'UPDATE `tsvn`.`lastrevision` SET `name` = \'processing\', `revision`=`revision`+1 WHERE CONVERT(`lastrevision`.`name` USING utf8) = \'processed\' LIMIT 1;';
$res=mysql_query($sql, $db);
if ($res===false) {
	echo "<br>\n<i>$query</i> <b>".mysql_error()."</b>";
	die ("In progress ?");
} else {
	var_dump($res);
}

echo "<hr>";
$sql = 'SELECT `revision` FROM `tsvn`.`lastrevision` WHERE CONVERT(`lastrevision`.`name` USING utf8) = \'processing\' LIMIT 1;';
$res=mysql_query($sql, $db);
if ($res===false) {
	echo "<br>\n<i>$query</i> <b>".mysql_error()."</b>";
	die ("In progress ?");
} else {
	$table=new Table;
	$table->importMysqlResult($res);
	$processingRev=$table->data[0][0]+0;
}

// read file
$file=file($dir."svn.info");

$info=array();
$index=0;
foreach ($file as $line) {
//	echo "<u>$line</u><br>\n";

	$line=trim($line);
	if ($line=="") {
		//if (!is_empty($info[count($info)]) 
		{
			$index++;
			//$info[]=array();
		}
		echo "<hr>";
		continue;
	}

	// fix Date> string fo Date:
	if (strpos("date>", $line)==0) {
		$line=str_replace("date>", "Date : ", $line);
	}
// separate parameter name and name

	// separate by strpos and substr
	$separatorpos=strpos($line, ":");
	$parameter=trim(substr($line, 0, $separatorpos));
	$value=trim(substr($line, $separatorpos+1));
//	echo "<b>$parameter</b> : <i>$value</i> <br>\n";

	// separate by explode
//	list($parameter, $value)=explode(":", $line, 2);
//	$parameter=trim($parameter);
//	$value=trim($value);
//	echo "<b>$parameter</b> : <i>$value</i> <br>\n";

	// separate by regexp

	// store values
	$info[$index][$parameter]=$value;
	echo "<b>$parameter</b> : <i>$value</i> <br>\n";
}


$currentRev=$info[0]["Current Rev"]+0;
$lastchangedRev=$info[1]["Last Changed Rev"]+0;

echo "currentRev: $currentRev<br>\n";
echo "lastchaRev: $lastchangedRev<br>\n";
echo "processingRev: $processingRev<br>\n";

if ($processingRev!=$currentRev) {
	die("not correct revision");
}

if ($lastchangedRev==$processingRev) {


# update revision info
$date=$info[0]["Date"];
$sqldate=trim(str_replace(array("Z", "T"), " ", $date));
$sql = "REPLACE `tsvn`.`revisions` SET `revision` = '$currentRev', `date`='$sqldate'";
$res=mysql_query($sql, $db);
if ($res===false) {
	echo "<br>\n<i>$sql</i> <b>".mysql_error()."</b>";
	die ("In progress ?");
} else {
	var_dump($res);
}
$sql = "ALTER TABLE `revisions` ORDER BY `revision`";
$res=mysql_query($sql, $db);

# fill more info about files
foreach ($info as &$record) {
	if (!isset($record["Name"]) || $record["Node Kind"]!=="file") {
		continue;
	}
	echo "<h2>".$record["Name"]."</h2>\n";
	$path=$record["Path"];
	echo "<br /><b>Path:</b> <i>$path</i>";
	if (preg_match("/(.*)\/((Tort|doc)[^_]*)?_?(..(_...?)?)?.(po[t]?)/", $path, $matches)) {
		$path2=$matches[1];
		$record["Po_Path"]=$path2;
		$record["Po_Name"]=$matches[2];
		$record["Po_Lang"]=$matches[4];
		$record["Po_Type"]=$matches[6];
		if ($record["Po_Name"]=="doc") {
			$record["Po_Name"]="";
		}
		$record["Po_Updated"]=$record["Last Changed Rev"]==$record["Revision"];
	} else {
		continue;
	}

	$group="?"; // unknown
	$re_g="/Languages\\/Tortoise[_\\.]/";
	$re_m="/doc\\/(.*\\/)*(TortoiseMerge(_...?)*)\\./";
	$re_d="/doc\\/(.*\\/)*((..|doc|TortoiseSVN)(_...?)*)\\./";
	if (preg_match($re_g, $path)) {
		$group="g"; // gui
	} else if (preg_match($re_m, $path, $matches)) {
		$group="m"; // merge
	} else if (preg_match($re_d, $path, $matches)) {
		$group="d"; // doc
	}
	$record["Po_Group"]=$group;
	echo "<br /><b>Group:</b> <i>".$record["Po_Group"]."</i>";
	echo "<br /><b>Lang:</b> <i>".$record["Po_Lang"]."</i>";
}


# check all pot files
echo "<hr><h1>POT</h1>";
foreach ($info as &$record) {
	if ($record["Po_Type"]!=="pot") {
		continue;
	}

	echo "<h2>".$record["Name"]."</h2>";
	$po=new po;
	$po->Load($record["Path"], "");
	//$po->Load($dir.$record["Path"], "");
	$total=$po->getStringCount();
	$record["Po_File"]=$po;

	if ($record["Po_Updated"]) {
		echo "update";
		$group=$record["Po_Group"];
		$lang=$record["Po_Lang"];
		$po->BuildReport();

		$query="REPLACE INTO state (`revision`, `group`, `language`) VALUES($currentRev, '$group', '$lang')";
		echo "<li>".$query;
		$res=mysql_query($query, $db);
		if ($res===false) {
			echo "<br>\n<i>$query</i><b>".mysql_error()."</b><br>\n";
		}
		$errTypes=$po->GetErrorTypes();
		foreach ($errTypes as $type) {
			$count=$po->GetErrorCount($type);
			echo "<br>\n<b>$type=".$count."</b>";
			$query="UPDATE state SET `$type`=$count WHERE `revision`=$currentRev AND `group`='$group' AND `language`='$lang' LIMIT 1";
			echo "<li>".$query;
			$res=mysql_query($query, $db);
			if ($res===false) {
				echo "<br>\n<i>$query</i> <b>".mysql_error()."</b><br>\n";
			}
		}
		echo "<br>\n";
	}
}

# check all po files
echo "<hr><h1>PO</h1>";
foreach ($info as &$record) {
	if ($record["Po_Type"]!=="po") {
		continue;
	}

	echo "<h2>".$record["Name"]."</h2>";
	//echo "<pre>";
	//var_dump($record);
	//echo "</pre>";

	// find corrensponding pot file
	$potrecord=NULL;
	foreach ($info as $potrec) {
		if ($potrec["Po_Type"]!=="pot") {
			continue;
		}
		if ($potrec["Po_Path"]==$record["Po_Path"] 
				&& $potrec["Po_Name"]==$record["Po_Name"]) {
			$potrecord=$potrec;
			echo "<h3>".$potrecord["Name"]."</h3>";
		}
	}
	echo $potrecord;
	$pot=$potrecord["Po_File"];



	if ($record["Po_Updated"] || $potrecord["Po_Updated"]) {


		$po=new po;
		//$po->Load($dir.$record["Path"], "");
		$po->Load($record["Path"], "");
		$total=$po->getStringCount();
		$record["Po_File"]=$po;


		$po->BuildReport($pot);
		echo "<br>\nupdate";
		$translated=0;
		$group=$record["Po_Group"];
		$lang=$record["Po_Lang"];
		$count=$po->GetErrorCount($type);
		$query="REPLACE INTO state (`revision`, `group`, `language`) VALUES($currentRev, '$group', '$lang')";
		echo "<li>".$query."</li>";
		$res=mysql_query($query, $db);
		if ($res===false) {
			echo "<br>\n<i>$query</i> <b>".mysql_error()."</b><br>\n";
		}
		$errTypes=$po->GetErrorTypes();
		foreach ($errTypes as $type) {
			$count=$po->GetErrorCount($type);
			echo "<br>\n<b>$type=".$count."</b>";
			$query="UPDATE state SET `$type`=$count WHERE `revision`=$currentRev AND `group`='$group' AND `language`='$lang' LIMIT 1";
			echo "<li>".$query;
			$res=mysql_query($query, $db);
			if ($res===false) {
				echo "<br>\n<i>$query</i> <b>".mysql_error()."</b><br>\n";
			}
		}
		echo "<br>\n";
	}
}

//PrintArray($info);
echo "<hr>";
//echo "<pre>";
//var_dump($info);
//echo "</pre>";

}

$sql = "UPDATE `tsvn`.`lastrevision` SET `name` = 'processed', `revision`=$currentRev WHERE CONVERT(`lastrevision`.`name` USING utf8) = 'processing' LIMIT 1;";
$res=mysql_query($sql, $db);
if ($res===false) {
	echo "<br>\n<i>$sql</i> <b>".mysql_error()."</b>";
	die ("In progress ?");
} else {
	var_dump($res);
}


php?>
