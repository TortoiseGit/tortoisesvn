<?php
define("MODULE_PATH", "../modules/");
define("EXT_PATH", MODULE_PATH."ext/");

#include("../scripts/session.php");
#include("mysql.php");

// common modules
include_once (EXT_PATH."Html.php");
#include_once (EXT_PATH."tools.php");
#include_once (EXT_PATH."Table.php");
#include_once (MODULE_PATH."csv.php");
#require_once (MODULE_PATH."lib/parsecsv/parsecsv.lib.php");

// LOG USER
	$log['AppName']="SVN";
	//Log_MailAccess($log);
	// get host
	$ip=$_SERVER["REMOTE_ADDR"];
	$agent=$_SERVER["HTTP_USER_AGENT"];
	$host=exec("host $ip");
	$host=substr($host, strrpos($host, " "));
	$host=trim($host);
	if ($host=="3(NXDOMAIN)") {
		$host="unknown";
	} else {
		$_SERVER["REMOTE_NAME"]="$host";
	}

	ob_start();
	echo "<html>";
	ksort($_SERVER);
	PrintArray($_SERVER);
	$dumpServer=ob_get_contents();
	echo "</html>";
	ob_end_clean();
	$to="otik@printflow.eu";
	$sub="SVN:$lang $ip $host";


	$msg=$msg."\n\n\n".$dumpServer;
	$ahead="";
	$ahead.="MIME-Version: 1.0\r\n";
	$ahead.="Content-type: text/html; charset=UTF-8\r\n";

	$crawler=preg_match("/(crawl|msnbot|yandex\.ru)/", $host) || preg_match("/(msnbot|spider)/", $agent);

	if (!preg_match("/192\.168\.10\.2|217\.75\.82\.141/", $_SERVER["REMOTE_ADDR"]) && !$crawler) {
		mail($to, $sub, $msg, $ahead);
	}
