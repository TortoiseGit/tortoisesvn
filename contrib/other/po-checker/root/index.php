<?php
include("mysql.php");

// how to extract module path to separate constant ?
// common modules
include_once ("../modules/ext/html.php");
include_once ("../modules/ext/tools.php");
include_once ("../modules/ext/table.php");

// local modules
include_once "../modules/po.php";
include_once "../modules/rc.php";

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

$msg=$_POST["msg"]; // opinion message
if ($msg=="") {
	$msg="";
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
	$sub="SVN:$lang".((isset($msg) && $msg!="") ? "+OPINION" : "")." $ip $host";


	$msg=$msg."\n\n\n".$dumpServer;
	$ahead="";
	$ahead.="MIME-Version: 1.0\r\n";
	$ahead.="Content-type: text/html; charset=UTF-8\r\n";

	if (!preg_match("/192.168.10.2/", $_SERVER["REMOTE_ADDR"]) && !preg_match("/(crawl.*googlebot.com|crawl.yahoo.net|crawler.*ask.com|msnbot.*search.msn.com)/", $host)) {
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
	<p>This tools is in development. It should help you to locate problems in translation files.</p>
	<p>Oto</p><hr />
	<form action=<?php echo "\"".htmlspecialchars($_SERVER["REQUEST_URI"])."\""; php?> method="post">
	Please fill your opinion about this page here.<br />
	<textarea name="msg" cols="55" rows="3"></textarea><br />
	<input type="submit" value="Send" />
	</form>
	<hr />

<?php
// find translations

//*/
$dirBase="/var/www/sites/tsvn.e-posta.sk/data/";
include_once $dirBase."trunk/contrib/drupal-modules/translation-status/trans_countries.inc";
include_once $dirBase."trunk/contrib/drupal-modules/translation-status/tortoisevars.inc";



echo "
<h1>For new translators</h1>
If you want to help with new translations:
<ul>
<li><i><b>0</b></i> Check if language you want to translate needs other translator - write to <i>translators at tortoisesvn dot net</i>.</li>
<li><i><b>1</b></i> Install <a href=\"http://poedit.sourceforge.net/\">poEdit</a> on your PC , or other tranlator tool.</li>
<li><i><b>2</b></i> Check out http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/Languages You'll get languages.
	<ul>
	<li><i><b>2.A</b></i> If you continue your language wil be already there</li>
	<li><i><b>2.B</b></i> Copy Tortoise.pot to Toroise_<i>langcode</i>.po. Where <i>langcode</i> is by ISO... </li>
	</ul>
</li>
<li><i><b>3</b></i> Update from .pot file and translate  :) </li>
<li><i><b>4</b></i> Send your translated file by either
	<ul>
	<li><i><b>4.A</b></i> Send your translated file to the developer list</li>
	<li><i><b>4.B</b></i> Create an account at tigris.org and send Luebbe (or <i>translators at tortoisesvn dot net</i>) your user name, so that I can give you commit access.</li>
	</ul>
</li>
<li><i><b>5</b></i> Check your translation on this page.</li>
</ul>
<i><b>Note:</b></i> Don't try to finish everything in one go. It takes a while and can become frustrating. Translate 5-50 phrases per day and check in these small pieces of work. (my personal recommendation)  :)
<br /><br /><hr />
";


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
echo "Last update: ".date("F d Y H:i", filemtime($revFileName))." GMT+1<br />";
if ($stable) {
	echo 'Go to <a href="/?l='.$lang.'">TRUNK</a>.<br /><br />';
} else {
	echo 'Go to <a href="/?stable=1&amp;l='.$lang.'">STABLE</a>.<br /><br />';
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
		<td><acronym title="Native language name in English">Language</acronym></td>
		<td>Native name</td>
		<td>Flag</td>
		<td><acronym title="Parameter test (Severity: High - may be harmfull)">PAR!!</acronym></td>
		<td><acronym title="Accelerator test (Severity: Medium - accessibility)">ACC!</acronym></td>
		<td><acronym title="New line style, count test (Severity: Low - appearance)">NLS</acronym></td>
		<td><acronym title="Untranslated (Severity: Low - appearance)">UNT</acronym></td>
		<td><acronym title="Fuzzy mark test (Severity: Low - appearance)">FUZ</acronym></td>
		<td><acronym title="Escaped chars (Severity: Low - appearance)">ESC</acronym></td>
<!--		<td><acronym title="TEST">TEST</acronym></td> -->
<!--		<td><acronym title="Spell check (Severity: Low - appearance)">PSPELL</acronym></td> -->
		<td>Doc</td>
		<td>Note:</td>
		<td>Author</td>
	</tr></thead>
	<tbody>';

	function PrintErrorCount($po, $name, $code, $stable) {
		$error=0;
		if (is_array($po)) {
			$error=$po[$name];
		} else if (is_a($po, "po")) {
			$error=$po->GetErrorCount($name);
		}
		if ($error===false) {
			echo "<td>?</td>\n";
		} else if (!$error) {
			echo "<td />\n";
		} else {
			echo "<td><a href=\"?stable=$stable&amp;l=$code#$name$code\" class=\"linkerror\">".$error."</a></td>\n";
		}
		return $error;
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
	foreach ($linesLoaded as $line) {
		if (preg_match_all("/(#)?([a-zA-Z_]*);\t([0-9]*);\t([0-9]*);\t([^;\t]*)[;\t]*([^;\t]*)[;\t]*([^;\t]*)[;\t]*/", $line, $matches)) {
			$code=$matches[2][0];
			$pos[$code]=NULL;
			$fileGui=$dirLocation."/Tortoise_$code.po";
			$fileSvn=$dirDoc."/TortoiseSVN_$code.po";
			$fileMerge=$dirDoc."/TortoiseMerge_$code.po";
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
			if (file_exists($file) && ($lang=="" || $lang==$code) && ($_SERVER["REMOTE_ADDR"]!="192.168.10.234b")) {
				$class=$classes[$classIndex];
				$classIndex=($classIndex+1)%count($classes);
				if (isset($countries[$code])) {
					$flagcode=$countries[$code][2];
				} else {
					$flagcode=$code;
				}
				echo "<tr class=\"$class\">";
				echo "<td>".$matches[6][0]."</td>\n";
				echo "<td>".$matches[7][0]."</td>\n";
				$imagesrc="http://tortoisesvn.net/flags/world.small/$flagcode.png";
				$imagesrc2=$langtocolor[$flagcode];
				if (isset($imagesrc2)) {
					$imagesrc2="/images/flags/$imagesrc2.png";
					echo "<td><a href=\"?stable=$stable&amp;l=$code#TAB$code\"><img src=\"$imagesrc2\" alt=\"$code\" height=\"48\" width=\"48\" /></a></td>\n";
				} else {
					echo "<td><a href=\"?stable=$stable&amp;l=$code#TAB$code\"><img src=\"$imagesrc\" alt=\"$code\" height=\"24\" width=\"36\" /></a></td>\n";
				}
				unset($po);
				if (!$langSelected && !$stable && $m='g') { // this i a hack ! - redesign !
					$query="SELECT * FROM `state` WHERE `language`='$code' AND `group`='g' ORDER BY `revision` DESC LIMIT 1";
					$res=mysql_query($query, $db);
					if ($res===false) {
						echo "<br /><i>$query</i> <b>".mysql_error()."</b>";
					} else if (mysql_num_rows($res)) {
						$table=new Table;
						$table->importMysqlResult($res);
						for ($i=0; $i<count($table->header); $i++) {
							$po[$table->header[$i]]=$table->data[0][$i];
						}
					}
				}
				if (!isset($po)) {
					$po=new po;
					$po->Load($file, $code);
					$po->AddPot($pot);
					$dicts=array();
//					$dicts[]="/var/www/sites/tsvn.e-posta.sk/data/trunk/doc/Aspell/TortoiseSVN.tmpl.pws";
					$dicts[]="/var/www/sites/tsvn.e-posta.sk/data/trunk/doc/Aspell/$lang.pws";
					$po->SetSpellingFiles($dicts);
					$po->buildReport();
				}
				$errorCount=0;
				$errorCount+=PrintErrorCount($po, "par", $code, $stable);
				$errorCount+=PrintErrorCount($po, "acc", $code, $stable);
				$errorCount+=PrintErrorCount($po, "nls", $code, $stable);
				$errorCount+=PrintErrorCount($po, "unt", $code, $stable);
				$errorCount+=PrintErrorCount($po, "fuz", $code, $stable);
				$errorCount+=PrintErrorCount($po, "esc", $code, $stable);
//				$errorCount+=PrintErrorCount($po, "spl", $code, $stable);

				$statDoc="";
				if (file_exists($fileSvn)) {
					unset($poTemp);
					if (!$langSelected && !$stable) { // this i a hack ! - redesign !
						$query="SELECT * FROM `state` WHERE `language`='$code' AND `group`='d' ORDER BY `revision` DESC LIMIT 1";
						$res=mysql_query($query, $db);
						if ($res===false) {
							echo "<br /><i>$query</i> <b>".mysql_error()."</b>";
						} else if (mysql_num_rows($res)) {
							$table=new Table;
							$table->importMysqlResult($res);
							for ($i=0; $i<count($table->header); $i++) {
								$poTemp[$table->header[$i]]=$table->data[0][$i];
							}
						}
						$unt=$poTemp["unt"];
						$fuz=$poTemp["fuz"];
					}
					if (!isset($poTemp)) {
						$poTemp=new po;
						$poTemp->Load($fileSvn, NULL);
						$poTemp->AddPot($potSvn);
						$poTemp->buildReport();
						$unt=$poTemp->GetErrorCount("unt");
						$fuz=$poTemp->GetErrorCount("fuz");
					}
					$statDoc.=" <a href=\"/?stable=$stable&amp;l=$lang&amp;m=s\">TSVN:</a>";
					if ($unt+$fuz==0) {
						$statDoc.="OK";
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
				}
				if (file_exists($fileMerge)) {
					unset($poTemp);
					if (!$langSelected && !$stable) { // this i a hack ! - redesign !
						$query="SELECT * FROM `state` WHERE `language`='$code' AND `group`='m' ORDER BY `revision` DESC LIMIT 1";
						$res=mysql_query($query, $db);
						if ($res===false) {
							echo "<br /><i>$query</i> <b>".mysql_error()."</b>";
						} else if (mysql_num_rows($res)) {
							$table=new Table;
							$table->importMysqlResult($res);
							for ($i=0; $i<count($table->header); $i++) {
								$poTemp[$table->header[$i]]=$table->data[0][$i];
							}
						}
						$unt=$poTemp["unt"];
						$fuz=$poTemp["fuz"];
					}
					if (!isset($poTemp)) {
						$poTemp=new po;
						$poTemp->Load($fileMerge, NULL);
						$poTemp->AddPot($potMerge);
						$poTemp->buildReport();
						$unt=$poTemp->GetErrorCount("unt");
						$fuz=$poTemp->GetErrorCount("fuz");
					}
					$statDoc.=" <a href=\"/?stable=$stable&amp;l=$lang&amp;m=m\">Merge:</a>";
					if ($unt+$fuz==0) {
						$statDoc.="OK";
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
				}
				unset($poTemp);
				if ($statDoc=="") {
					$statDoc="-";
				}
				if ($matches[4][0]=="1") {
					echo "<td>:$statDoc</td>";
					
				} else {
					echo "<td>$statDoc</td>\n";
				}
				if ($matches[1][0]=="#") {
					echo "<td>Disabled</td>";
				} else if(!$errorCount) {
					echo "<td>OK</td>\n";
				} else {
					echo "<td />\n";
				}
				echo "<td>".$countries[$code][4]."</td>";
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
HtmlFooter($HtmlType);
//php?>
