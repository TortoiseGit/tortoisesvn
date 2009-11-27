<?php
define("MODULE_PATH", "../modules/ext/");
include("../scripts/session.php");
include("mysql.php");


// how to extract module path to separate constant ?
// common modules
include_once (MODULE_PATH."html.php");
include_once (MODULE_PATH."tools.php");
include_once (MODULE_PATH."table.php");

// local modules
include_once "../modules/po.php";
include_once "../modules/rc.php";

$iconSize="32x32";
$icons=array(
	'ok' => "/images/classy/$iconSize/accept.png",
	'error' => "/images/classy/$iconSize/delete.png",
	'warning' =>"/images/classy/$iconSize/warning.png",
	'unknown' => "/images/classy/$iconSize/help.png",
	'doc' => "/images/classy/$iconSize/folder.png",
	'new' => "/images/classy/$iconSize/favorite.png",
	'note' => "/images/classy/$iconSize/notebook.png",
	'down' => "/images/classy/$iconSize/down.png",
	'go' => "/images/classy/$iconSize/next.png",
	'disabled' => "/images/classy/$iconSize/remove.png",
	'language' => "/images/classy/$iconSize/page.png",
	'flag' => "/images/classy/$iconSize/globe.png",
	'author' => "/images/classy/$iconSize/image.png",
	'authors' => "/images/classy/$iconSize/users.png",
	'info' => "/images/classy/$iconSize/info.png",
	'gui' => "/images/classy/$iconSize/application.png"
	);
// predefine header info
$title="TortoiseSVN | The coolest Interface to (Sub)Version Control - Translation check";
$css="tsvn.css";

$HtmlType="xhtml10t";
unset($HtmlTitle);
$HtmlTitle="TortoiseSVN | The coolest Interface to (Sub)Version Control - Translation check";
unset($HtmlScript);
$HtmlScript=array(array(""=>""));
unset($HtmlStyle);
$HtmlStyle[]=array("type"=>"text/css", "media"=>"all", "/"=>'@import "/css/tsvn.css";');
$HtmlStyle[]=array("type"=>"text/css", "media"=>"print", "/"=>'@import "/css/print.css";');
$HtmlHeader=array("title"=>$HtmlTitle, "script"=>$HtmlScripts, "style"=>$HtmlStyle);
$HtmlBody=array("class"=>"sidebars");
HtmlHeader($HtmlType, $HtmlHeader, $HtmlBody);


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


unset($forceCode);

if (isset($_FILES['uploadedfile']['name'])) {
	$uploadedfile=$_POST["uploadedfile"];

	$target_path="../temp/";

	/* Add the original filename to our target path.
	Result is ""uploads/filename.extension" */
	$target_path = $target_path . basename( $_FILES['uploadedfile']['name']) . ".tmp" . session_id();

	if (move_uploaded_file($_FILES['uploadedfile']['tmp_name'], $target_path)) {
		$uploadresult="<br />The file" . basename( $_FILES['uploadedfile']['name']) . " has been uploaded";
		$baseName=$_FILES['uploadedfile']['name'];
		if (preg_match_all("/(^[a-zA-Z]*)_([a-zA-Z_]*)\.(.*)/", $baseName, $matches)) {
			echo "<pre>";
//			var_dump($matches);
			echo "</pre>";
			$forceCode=$matches[2][0];
			$lang=$forceCode;
			$forcePoFileName=$target_path;
			$forceMode='g';
			echo "baseName=$baseName";
			if (strpos($baseName, "TortoiseSVN")!==false) {
				echo "SVN";
				$forceMode='s';
			} else if (strpos($baseName, "TortoiseMerge")!==false) {
				echo "merge";
				$forceMode='m';
			}
		}
		echo "<hr />forceCode : $forceCode <hr />";
	} else {
		$uploadError="<br />There was an error uploading file, please try again!";
	}
}





	// get host
	$ip=$_SERVER["REMOTE_ADDR"];
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

	if (!preg_match("/192.168.10.2/", $_SERVER["REMOTE_ADDR"]) && !preg_match("/(crawl|msnbot)/", $host)) {
		mail($to, $sub, $msg, $ahead);
	}//*/

php?>

	<div id="header-region" class="clear-block"></div>
	<div id="wrapper">
		<div id="container" class="clear-block">

			<div id="header">
				<div id="logo-floater">
					<h1><a href="/" title="TortoiseSVN The coolest Interface to (Sub)Version Control"><img src="/images/tsvn_logo.png" alt="TortoiseSVN The coolest Interface to (Sub)Version Control" id="logo" /><span>TortoiseSVN</span> The coolest Interface to (Sub)Version Control</a></h1>
				</div>
			</div> <!-- /header -->
		</div>
	</div>

<div>
<br />
<br />
<br />
<br />
<h1>TSVN poChekcer</h1>
	<p>This tool should help you to locate problems in translation files. Even lot of test works now this tools is still in development.</p>
	<p>This tool start for fills need in finding missing accelerator - Luebbe's translation status page shows count but not string so it was no so easy to find affected string and fix it.
	After some time this tool suite expand to current state. It is long term task, and is always in devepement. Anyway I hope is halp you to increase translation quality.</p>
	<p>Oto</p><hr />
	<hr />

<h1>For new translators</h1>
<p>Please follow this link if you think you may help with translation.<br />
<a href="http://tortoisesvn.net/translation">-> TortoiseSVN translation</a></p><hr />

<h1>Precommit checker</h1>
<p>You can upload file for check before you make actual commit, so you can fix errors in one commit.</p>
<p>Make sure you are going to check translation against proper branch.</p>
 <form enctype="multipart/form-data" action="index.php" method="post">
 <input type="hidden" name="MAX_FILE_SIZE" value="2048000" />
 Choose a file to upload: <input name="uploadedfile" type="file" /><br />
 <input type="submit" value="Upload File" />
 </form>
<?php
	echo $uploadresult;
	echo $uploadError;
php?>
 <br />
 <br />

<hr />

<?php
// find translations

//*/
$dirBase="/srv/www/sites/tsvn.e-posta.sk/data/";
$transCountriesFile=$dirBase."trunk/contrib/drupal-modules/translation-status/trans_countries.inc";
if (file_exists($transCountriesFile)) {
	include_once $transCountriesFile;
}
$tortoiseVarsFile=$dirBase."trunk/contrib/drupal-modules/translation-status/tortoisevars.inc";
if (file_exists($tortoiseVarsFile)) {
	include_once $tortoiseVarsFile;
}
$tortoiseVarsFile=$dirBase."trunk/contrib/other/drupal-modules/translation-status/tortoisevars.inc";
if (file_exists($tortoiseVarsFile)) {
	include_once $tortoiseVarsFile;
}





$dirTarget="trunk";
$dir=$dirBase.$dirTarget;
$revFileName=$dir.$target.".rev";
if ($_GET["stable"]) {
	echo "Using BRANCH";
	$dirTarget="branches/1.6.x";
	$revFileName=$dirBase."branches.rev";
	$dir=$dirBase.$dirTarget;
}

$dirLocation=$dir."/Languages";
$dirDoc=$dir."/doc/po";
$revFileLines=file($revFileName);

echo "<h1>".$revFileLines[0]."of $dirTarget</h1>";
if (false) { // if replaying
	$rev=substr($revFileLines[0], strpos($revFileLines[0], " "))+0;
	$sec=(16330-$rev)*8;
	$min=round($sec*10/60)/10;
	$hour=round($min*10/60)/10;
	if ($hour>=1) {
		$timeToGo=$hour."h";
	} else if ($min>=1) {
		$timeToGo=$min."m";
	} else if ($sec>0) {
		$timeToGo=$sec."s";
	}
	echo "<b>!!! PLEASE return in couple of minutes - trunk replay in progress !!! Appx. $timeToGo to go.</b> - branch is working -<br />";
}
echo "<p>Last update: ".date("F d Y H:i", filemtime($revFileName))." CET (GMT+1/GMT+2(DST)) <br />";
if ($stable) {
	echo 'Go to <a href="/?l='.$lang.'">TRUNK</a>.</p>';
} else {
	echo 'Go to <a href="/?stable=1&amp;l='.$lang.'">STABLE</a>.</p>';
}


	$potGui=new po;
	$potGui->load("$dirLocation/Tortoise.pot", NULL);
	$potMerge=new po;
	$potMerge->load("$dirDoc/TortoiseMerge.pot", NULL);
	$potSvn=new po;
	$potSvn->load("$dirDoc/TortoiseSVN.pot", NULL);
	$pos=array();

	$data=array();

// load language.txt
	// open_file & load lines
	$linesLoaded=file("$dirLocation/Languages.txt");
	// iterate over lines
	unset($msgid);
	$lastLineType=PO_NONE;
	$linenum=0;
	echo "<a name=\"TAB$lang\"></a>";
	echo '<table border="1"><thead><tr>
		<td rowspan="2"><acronym title="Native language name in English"><img src="'.$icons['language'].'" alt="" />Language</acronym></td>
		<td colspan="8"><img src="'.$icons['gui'].'" alt="" />GUI check</td>
		<td colspan="2"><img src="'.$icons['doc'].'" alt="" />DOC</td>
		<td rowspan="2"><img src='.$icons['authors'].' alt="" />Author(s)</td>
	</tr><tr>
		<td><img src="'.$icons['flag'].'" />Flag</td>
		<td><acronym title="Parameter test (Severity: High - may be harmfull)"><img src="'.$icons['error'].'" alt="" />PAR!!</acronym></td>
		<td><acronym title="Accelerator test (Severity: Medium - accessibility)"><img src="'.$icons['info'].'" alt="" />ACC!</acronym></td>
		<td><acronym title="New line style, count test (Severity: Low - appearance)"><img src="'.$icons['warning'].'" alt="" />NLS</acronym></td>
		<td><acronym title="Untranslated (Severity: Low - appearance)"><img src="'.$icons['new'].'" alt="" />UNT</acronym></td>
		<td><acronym title="Fuzzy mark test (Severity: Low - appearance)"><img src="'.$icons['unknown'].'" alt="" />FUZ</acronym></td>
		<td><acronym title="Escaped chars (Severity: Low - appearance)"><img src="'.$icons['info'].'" alt="" />ESC</acronym></td>
		<td><img src="'.$icons['note'].'" alt="" />Note</td>
		<td><img src="/images/32/tsvn.jpg" title="TortoiseSVN DOC" /></td>
		<td><img src="/images/32/tmerge.jpg" title="TortoiseMerge DOC" /></td>
	</tr></thead>
	<tbody>';

	function ExtractErrorCount($po, $name) {
		if (is_array($po)) {
			return $po[$name];
		} else if (is_a($po, "po")) {
			return $po->GetErrorCount($name);
		}
		return false;
	}

	function GetCheckResulForLanguage($fileName, $lang, $pot, $db, $group) {
		if (!file_exists($fileName)) {
			return false;
		}
		unset($poTemp);
		if (isset($db) && isset($group) && $db!=NULL && $group!="") { // this i a hack ! - redesign !
			$query="SELECT * FROM `state` WHERE `language`='$lang' AND `group`='$group' ORDER BY `revision` DESC LIMIT 1";
			$res=mysql_query($query, $db);
			if ($res===false) {
				echo "<br /><i>$query</i> <b>".mysql_error()."</b>";
			} else if (mysql_num_rows($res)) {
				$table=new Table;
				$table->importMysqlResult($res);
				for ($i=0; $i<count($table->header); $i++) {
					$poTemp[$table->header[$i]]=$table->data[0][$i];
				}
				return $poTemp;
			}
		}
		$poTemp=new po;
		$poTemp->Load($fileName, $lang);
		$poTemp->AddPot($pot);
		$poTemp->buildReport();
		return $poTemp;
	}

	function PrintErrorCount($po, $name, $code, $stable) {
		$errorCount=ExtractErrorCount($po, $name);
		if (is_array($errorCount)) {
			$warningCount=$errorCount('warnings');
			$errorCount=$errorCount('errors');
			if (!isset($errorCount)) {
				$errorCount=0;
			}
		}
		if ($errorCount===false) {
			echo "<td>?</td>\n";
			return 0;
		} else if (!$errorCount && !$warningCount) {
			echo "<td />\n";
			return 0;
		}
		echo "<td>";
		if ($errorCount) {
			echo " <a href=\"?stable=$stable&amp;l=$code#$name$code\" class=\"linkerror\">".$errorCount."</a> ";
		}
		if ($warningCount) {
			echo " <a href=\"?stable=$stable&amp;l=$code#$name$code\" class=\"linkwarning\">".$warningCount."</a> ";
		}
		echo "</td>\n";
		return $errorCount;
	}

	function GetDocStatus($fileName, $lang, $pot, $db=NULL, $group=NULL) {
		$po=GetCheckResulForLanguage($fileName, $lang, $pot, $db, $group);
		if (!isset($po) || $po==NULL) {
			return "-";
		}
		$unt=ExtractErrorCount($po, "unt");
		$fuz=ExtractErrorCount($po, "fuz");
		if ($unt+$fuz==0) {
			return "OK";
		} else {
			if ($unt) {
				if ($fuz) {
					$statDoc.="<b>$unt</b>/<i>$fuz</i>";
				} else {
					$statDoc.="<b>$unt</b>";
				}
			} else {
				$statDoc.="<i>$fuz</i>";
			}
		}
		return $statDoc;
	}
	$langtocolor=array(
			"sk" => "Slovakia",
			"cs" => "Czech%20Republic",
			"bg" => "Bulgaria",
			"fi" => "Finland",
			"sl" => "Slovenia",
			"sv" => "Sweden",
			"da" => "Denmark",
			"pl" => "Poland",
			"ko" => "South%20Korea",
			"hu" => "Hungary",
			"fr" => "France",
			"pt_BR" => "Brazil",
			"ru" => "Russian%20Federation",
			"sp" => "Spain",
			"nl" => "Netherlands",
			"zh_CN" => "China",
			"zh_TW" => "Taiwan",
			"id" => "Indonezia",
			"uk" => "Ukraine",
			"pt_PT" => "Portugal",
			"de" => "Germany",
			"sr_spc" => "Serbia(Yugoslavia)",
			"sr_spl" => "Serbia(Yugoslavia)",
			"tr" => "Turkey",
			"ja" => "Japan",
			"hr" => "Croatia",
			"el" => "Greece",
			"ro" => "Romania",
			"es" => "Spain",
			"nb" => "Norway",
			"it" => "Italy",
			"mk" => "Macedonia",
			"fa" => "Iran",
			"ka" => "Georgia",
			"" => "",
			"" => "");

	$langSelected=($lang!="");
	$classIndex=0;
	$classes=array("odd", "even");
	unset($countries);
	$countriesFile="trans_countries.inc";
	$countriesFile=$dir."/contrib/drupal-modules/translation-status/".$countriesFile;
	if (file_exists($countriesFile)) {
		include $countriesFile;
	}

	function findCoutriesParam($countries, $code, $index, $default) {
		$ret=$default;
		if (isset($countries[$code]) && true) {
			return $countries[$code][$index];
		}
		return $ret;
	}

	foreach ($linesLoaded as $line) {
		if (preg_match_all("/(([#01-]*)?);[ \t]*([a-zA-Z_]*);[ \t]*([0-9]*);[ \t]*([0-9]*);[ \t]*([^;\t]*)[;\t]*([^;\t]*)[;\t]*([^;\t]*)[;\t]*/", $line, $matches)) {
//			echo "<pre>"; var_dump($matches); echo "</pre>";
			$code=$matches[3][0];
			$language=$stable ? $matches[7][0] : ($matches[6][0]);
			$language=findCoutriesParam($countries, $code, 3, $language);
			$author=$stable ? $matches[9][0] : ($matches[7][0]);
			$author=findCoutriesParam($countries, $code, 4, $author);
			$author=trim($author, " \"\r\n");
			$pos[$code]=NULL;
			$fileGui=$dirLocation."/Tortoise_$code.po";
			$fileSvn=$dirDoc."/TortoiseSVN_$code.po";
			$fileMerge=$dirDoc."/TortoiseMerge_$code.po";
			if (isset($forceCode)) {
				if ($forceCode!=$code) {
					continue;
				}
				$m=$forceMode;
				$fileGui=$forcePoFileName;
				$lang=$forceCode;
				echo "lang=$lang";
				echo "m=$m";
			}
			switch ($m) {
			 default:
			 case 'g':
				$file=$fileGui;
				$pot=$potGui;
				break;

			 case 's':
				$file=$fileSvn;
				$pot=$potSvn;
				break;

			 case 'm':
				$file=$fileMerge;
				$pot=$potMerge;
				break;
			}
			if (file_exists($file) && ($lang=="" || $lang==$code)) {
				$class=$classes[$classIndex];
				$classIndex=($classIndex+1)%count($classes);
//				if (isset($countries[$code])) {
//					$flagcode=$countries[$code][2];
//				} else {
					$flagcode=$code;
//				}
				echo "<tr class=\"$class\">";
				echo "<td>".$language."</td>\n";
				$imagesrc="http://tortoisesvn.net/flags/world.small/$flagcode.png";
				$imagesrc2=$langtocolor[$flagcode];
				if (isset($imagesrc2)) {
					$imagesrc2="/images/flags/$imagesrc2.png";
					echo "<td><a href=\"?stable=$stable&amp;l=$code#TAB$code\"><img src=\"$imagesrc2\" alt=\"$code\" height=\"48\" width=\"48\" /></a></td>\n";
				} else {
					echo "<td><a href=\"?stable=$stable&amp;l=$code#TAB$code\"><img src=\"$imagesrc\" alt=\"$code\" height=\"24\" width=\"36\" /></a></td>\n";
				}
				if (!$langSelected && !$stable && $m='g') { // this i a hack ! - redesign !
					$dbLink=$db;
				} else {
					$dbLink=NULL;
				}
				$po=GetCheckResulForLanguage($file, $code, $pot, $dbLink, $m);
				$errorCount=0;
				$errorCount+=PrintErrorCount($po, "par", $code, $stable);
				$errorCount+=PrintErrorCount($po, "acc", $code, $stable);
				$errorCount+=PrintErrorCount($po, "nls", $code, $stable);
				$errorCount+=PrintErrorCount($po, "unt", $code, $stable);
				$errorCount+=PrintErrorCount($po, "fuz", $code, $stable);
				$errorCount+=PrintErrorCount($po, "esc", $code, $stable);
//				$errorCount+=PrintErrorCount($po, "spl", $code, $stable);

				// note
				if ($matches[1][0]=="-1") {
					echo '<td><img src="'.$icons['disabled'],'" alt="Disabled" title="Disabled"/></td>';
				} else if(!$errorCount) {
					echo '<td><img src="'.$icons['ok'],'" alt="o.k." title="OK"/></td>';
				} else {
					echo "<td />";
				}

				if (!$langSelected && !$stable) { // this i a hack ! - redesign !
					$dbLink=$db;
				} else {
					$dbLink=NULL;
				}

				$statDocTsvn=GetDocStatus($fileSvn, $code, $potSvn, $dbLink, 'd');
				if ($statDocTsvn=="-") {
				} else {
					if ($statDocTsvn=="OK") {
						$statDocTsvn='<img src="'.$icons['ok'].'" alt="o.k." title="OK"/>';
					}
					$statDocTsvn='<a href="/?stable='.$stable.'&amp;l='.$code.'&amp;m=s">'.$statDocTsvn.'</a>';
				}
				echo "<td>$statDocTsvn</td>\n";

				$statDocMerge=GetDocStatus($fileMerge, $code, $potMerge, $dbLink, 'm');
				if ($statDocMerge=="-") {
				} else {
					if ($statDocMerge=="OK") {
						$statDocMerge='<img src="'.$icons['ok'].'" alt="o.k." title="OK"/>';
					}
					$statDocMerge='<a href="/?stable='.$stable.'&amp;l='.$code.'&amp;m=m">'.$statDocMerge.'</a>';
				}
				echo "<td>$statDocMerge</td>\n";

				echo "<td>".$author."</td>";
				echo "</tr>\n";
				$pos[$code]=$po;
				$line=array($englishName, $nativeName, $flag, $par, $acc, $unt, $fuz, $dis, $doc);
				flush();
			}
		}
	}
	echo '</tbody></table>';


	if (is_dir($dirLocation)) {
		if ($dh = opendir($dirLocation)) {
			while (($file = readdir($dh)) !== false) {
				if ($file[0]==".") {
					continue;
				}
				if (is_dir($dirLocation . $file)) {
					continue;
				}
				if (preg_match_all("/^Tortoise_(.*)\.po$/", $file, $result, PREG_PATTERN_ORDER)) {
					$code=$result[1][0];
					if ($code==$lang) {
						$poFileName=$dirLocation."/".$file;
					}
				}
			}
			closedir($dh);
		}
	} else {
		$Loc="";
		echo "<b>!!! NO DIR FOUND - Please wait 5 minute and try again. Forced full update may be in progress.!!!</b><br />";
		exit;
	}//*/
#		$this->Name=$Name;
#		$this->Files=$Files;
#		$this->SubDirs=$SubDirs;
#		$this->Location=$Loc;

function IsHistory ($group, $lang) {
	global $db;
	$historyTest=true;
	//return true;
	include ("history.php");
	return $res;
}

if (!$stable && $lang!="") {
	foreach (array(array("g", "GUI"), array("d", "TSVN doc"), array("m", "TMerge doc")) as $group) {
		if (IsHistory($group[0], $lang)) {
			echo "<hr /><h1>".$group[1]." history graph:</h1><br />";
			echo "<img src=\"history.php?l=$lang&amp;g=".$group[0]."&amp;x=$gx\" alt=\"history\"/>";
		}
	}
}

echo "<hr />";
$revFileLine=file($revFileName);
echo "<h1>".$revFileLines[0]."</h1>";

$native=$pos[$lang];
if ($native) {
	echo "<a name=\"PO$lang\"></a><h2>PO Check ($lang)</h2>\n";
	$native->buildReportFor("spl");
	$native->printReport($pot);

	echo "\n
		<h1>RC Checks</h1>\n
		<p>Next few sections informs about duplicate accelerators in translation. 
		There is no reason to be stressed about this, but some translators like to know it. 
		In a fact even English translation contains duplicate.</p>\n
		<h2>Proc RC Check ($lang)</h2>
		";
	if (false && $lang=="sk") {
		$rc=new rc;
		$rc->load($dir."/src/Resources/TortoiseProcENG.rc");
		$rc->Translate($native);
		$rc->report();

		echo "<h2>Merge RC Check ($lang)</h2>";
		$rc=new rc;
		$rc->load($dir."/src/Resources/TortoiseMergeENG.rc");
		$rc->Translate($native);
		$rc->report();


		echo "<h2>Proc RC Check (EN)</h2>";
		$rc=new rc;
		$rc->load($dir."/src/Resources/TortoiseProcENG.rc");
		$rc->report();

		echo "<h2>Merge RC Check (EN)</h2>";
		$rc=new rc;
		$rc->load($dir."/src/Resources/TortoiseMergeENG.rc");
		$rc->report();
		//* /
	} else {
		echo "<p>RC checking is currently off for this language. If you like enable it for your translation drop me an email.</p>\n";
	}
} else {
	echo "<h1><font class=\"elerror\">Language not defined or not found !</font></h1>\n";
	echo "producing .pot tests";
	$pot->buildReport();
	$pot->PrintReport();
}//*/

echo '</div>';
echo '<div><center><hr/>Icons by: <a href="http://dryicons.com" class="link">DryIcons</a></center></div>';
HtmlFooter($HtmlType);
php?>
