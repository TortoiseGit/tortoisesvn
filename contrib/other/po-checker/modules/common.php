<?php // $Id$

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



// show version infos
$target=$dirTarget;
$dirBase="/srv/www/sites/tsvn.e-posta.sk/data/";
$dir=$dirBase.$dirTarget;
$revFileName=$dirBase."info/".$target.".rev";

$dirLocation=$dir."/Languages";
$dirDoc=$dir."/doc/po";
$revFileLines=file($revFileName);


//echo $revFileName;
echo "<h1>".$revFileLines[0]."of $targetDisplayName</h1>";
echo "<p>Last update: ".date("F d Y H:i", filemtime($revFileName))." CET (GMT+1/GMT+2(DST)) <br />";
if ($stable) {
	echo 'Go to <a href="/?l='.$lang.'">TRUNK</a>.</p>';
} else {
	echo 'Go to <a href="/?stable=1amp;l='.$lang.'">STABLE</a>.</p>';
}


function BuildLanguageList($languagelistFileName) {
	$langToFlag=array(
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
			"sr@latin" => "Serbia(Yugoslavia)",
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
			"ml_IN" => "India",
			"" => "");


	// load language.txt
	$csv = new parseCSV();
	// ...or if you know the delimiter, set the delimiter character if its not the default comma...
	$csv->delimiter = ";";
	// ...and then use the parse() function.
	$csv->parse($languagelistFileName);


	function AnalyseLanguagesTxtLine($csvLine) {
		if (!isset($csvLine) || !is_array($csvLine) || count($csvLine)<1) {
			return false;
		}
		$indexedArray=array_values($csvLine);
		if ($indexedArray[0][0]=='#') {
			return false;
		}
		$ret=array();
		switch (count($csvLine)) {
		 case 6:
		 	if (preg_match("/^[-01]/", $indexedArray[0])) {
				$ret['Enabled']=$indexedArray[0];
				$ret['Lang']=$indexedArray[1];
				$ret['Code']=$indexedArray[2];
				$ret['Flags']=$indexedArray[3];
				$ret['LangName']=$indexedArray[4];
				$ret['Credits']=$indexedArray[5];
			} else {
				$ret['Lang']=$indexedArray[0];
				$ret['Code']=$indexedArray[1];
				$ret['Enabled']=$indexedArray[2];
				//$ret['Flags']=$indexedArray[3];
				$ret['LangName']=$indexedArray[4];
				//$ret['Credits']=$indexedArray[5];
				// Credits should be loaded from other file ... (vars)
			}
			break;

		 case 7:
			$ret['Enabled']=$indexedArray[0];
			$ret['Lang']=$indexedArray[1];
			$ret['WixLang']=$indexedArray[2];
			$ret['Code']=$indexedArray[3];
			$ret['Flags']=$indexedArray[4];
			$ret['LangName']=$indexedArray[5];
			$ret['Credits']=$indexedArray[6];
		}
		return $ret;
	}


	$list=array();
	foreach ($csv->data as $row) {
		$rowArray=AnalyseLanguagesTxtLine($row);
		if ($rowArray===false) {
			continue;
		}
		$rowArray['Flag']=$langToFlag[$rowArray['Lang']];
		$list[]=$rowArray;
	}
	return $list;
}

$list=BuildLanguageList("$dirLocation/Languages.txt");

/*	$potGui=new po;
	$potGui->load("$dirLocation/Tortoise.pot", NULL);
	$potMerge=new po;
	$potMerge->load("$dirDoc/TortoiseMerge.pot", NULL);
	$potSvn=new po;
	$potSvn->load("$dirDoc/TortoiseSVN.pot", NULL);
	$potSvn=new po;
	$potSvn->load("$dirLocation/TortoiseDoc.pot", NULL);
	$pos=array();

	$data=array();//*/

	$potGui=new po;
	$potGui->load("$dirLocation/TortoiseUI.pot", NULL);
	$potDoc=new po;
	$potDoc->load("$dirLocation/TortoiseDoc.pot", NULL);



	// iterate over lines
	unset($msgid);
	echo "<a name=\"TAB$lang\"></a>";
	echo '<table border="1"><thead><tr>
		<td rowspan="2"><acronym title="Native language name in English"><img src="'.$icons['language'].'" alt="" />Language</acronym></td>
		<td colspan="8"><img src="'.$icons['gui'].'" alt="" />GUI check</td>
		<td colspan="3"><img src="'.$icons['doc'].'" alt="" />DOC</td>
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
		<td><img src="'.$icons['doc'].'" title="Tortoise DOC" /></td>
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
		if (isset($db) && isset($group) && $db!=NULL && $group!="") { // this is a hack ! - redesign !
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

	function PrintErrorCount($po, $name, $link="") {
		$errorCount=ExtractErrorCount($po, $name);
		if (is_array($errorCount)) {
			$warningCount=$errorCount('warnings');
			$errorCount=$errorCount('errors');
		}
		if ($errorCount===false || !isset($errorCount)) {
			echo "<td>?</td>\n";
			return 0;
		}
		if (!$errorCount && !$warningCount) {
			echo "<td />\n";
			return 0;
		}
		$link .= "#$name";
		echo "<td>";
		if ($errorCount) {
			echo " <a href=\"$link\" class=\"linkerror\"><img src=\"/images/classy/16x16/delete.png\" />".$errorCount."</a> ";
		}
		if ($warningCount) {
			echo " <a href=\"$link\" class=\"linkwarning\"><img src=\"/images/classy/16x16/warning.png\" />".$warningCount."</a> ";
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
		if (isset($countries[$code])) {
			return $countries[$code][$index];
		}
		return $ret;
	}


	// prefill variables used in loop
	$reportCodeList=array("par", "acc", "nls", "unt", "fuz", "esc"/*, "spl"*/);
/*	if (!$langSelected && !$stable) { // this is a hack ! - redesign !
		if ($m='g') { // this i a hack ! - redesign !
			$dbLink1=$db;
		} else {
			$dbLink1=NULL;
		}
		$dbLink2=$db;
	} else //*/ {
		$dbLink1=NULL;
		$dbLink2=NULL;
	}

	foreach ($list as $rowArray) {
		$state=$rowArray['Enabled'];
		$code=$rowArray['Lang'];
		$language=$rowArray['LangName'];
		$language=findCoutriesParam($countries, $code, 3, $language);
		$flag=$rowArray['Flags'];
		$rowArray['LangName'];
		$author=$rowArray['Credits'];
		$author=findCoutriesParam($countries, $code, 4, $author);

		$pos[$code]=NULL;
		$file=$dirLocation."/$code/TortoiseUI.po";
		$pot=$potGui;
		$fileDoc=$dirLocation."/$code/TortoiseDoc.po";
		$fileGui=$dirLocation."/Tortoise_$code.po";
		$fileSvn=$dirDoc."/TortoiseSVN_$code.po";
		$fileMerge=$dirDoc."/TortoiseMerge_$code.po";

/*		switch ($m) {
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
		}//*/;

		{
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
				$link="?stable=$stable&amp;m=$m&amp;l=$code";
				$imagesrc="http://tortoisesvn.net/flags/world.small/$flagcode.png";
				$imagesrc2=$flag;
				if (isset($imagesrc2)) {
					$imagesrc2="/images/flags/$imagesrc2.png";
					echo "<td><a href=\"$link#TAB$code\"><img src=\"$imagesrc2\" alt=\"$code\" height=\"48\" width=\"48\" /></a></td>\n";
				} else {
					echo "<td><a href=\"$link#TAB$code\"><img src=\"$imagesrc\" alt=\"$code\" height=\"24\" width=\"36\" /></a></td>\n";
				}
				$po=GetCheckResulForLanguage($file, $code, $pot, $dbLink1, $m);
				$errorCount=0;
				foreach ($reportCodeList as $reportCode) {
					$errorCount+=PrintErrorCount($po, $reportCode, $link);
				}

				// note
				if ($state=="-1") {
					echo '<td><img src="'.$icons['disabled'],'" alt="Disabled" title="Disabled"/></td>';
				} else if(!$errorCount) {
					echo '<td><img src="'.$icons['ok'],'" alt="o.k." title="OK"/></td>';
				} else {
					echo "<td />";
				}

				$statDoc=GetDocStatus($fileDoc, $code, $potDoc, $dbLink2, 'd');
				if ($statDoc=="-") {
				} else {
					if ($statDocTsvn=="OK") {
						$statDocTsvn='<img src="'.$icons['ok'].'" alt="o.k." title="OK"/>';
					}
					$statDocTsvn='<a href="/?stable='.$stable.'&amp;l='.$code.'&amp;m=s">'.$statDocTsvn.'</a>';
				}
				echo "<td>$statDoc</td>\n";

				$statDocTsvn=GetDocStatus($fileSvn, $code, $potSvn, $dbLink2, 'd');
				if ($statDocTsvn=="-") {
				} else {
					if ($statDocTsvn=="OK") {
						$statDocTsvn='<img src="'.$icons['ok'].'" alt="o.k." title="OK"/>';
					}
					$statDocTsvn='<a href="/?stable='.$stable.'&amp;l='.$code.'&amp;m=s">'.$statDocTsvn.'</a>';
				}
				echo "<td>$statDocTsvn</td>\n";
				$statDocMerge=GetDocStatus($fileMerge, $code, $potMerge, $dbLink2, 'm');
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
	}//* /
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
			echo "<img class=\"history\" src=\"history.php?l=$lang&amp;g=".$group[0]."&amp;x=$gx\" alt=\"history\"/>";
			echo "<img class=\"progress\" src=\"statusmap.php?l=$lang&amp;g=".$group[0]."&amp;x=$gx\" alt=\"progressmap\"/>";
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
}//* /

echo '</div>';
echo '<div><center><hr/>Icons by: <a href="http://dryicons.com" class="link">DryIcons</a></center></div>';
//php?>
