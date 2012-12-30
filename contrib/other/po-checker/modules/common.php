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


include_once("source.php");
include_once("po.php");


// show version infos
$target=$dirTarget;
$dirBase="/srv/www/sites/tsvn.e-posta.sk/data/";
$dirBase="/var/www/po-checker/data/";
$dir=$dirBase.$dirTarget;
$revFileName=$dirBase."info/".$target.".rev";

$dirLocation=$dir."/Languages";
$dirDoc=$dir."/doc/po";
$revFileLines=file($revFileName);


//echo $revFileName;
// render source menu
echo "<h1>".$revFileLines[0]."of $targetDisplayName</h1>";
echo "<p>Last update: ".date("F d Y H:i", filemtime($revFileName))." CET (GMT+1/GMT+2(DST)) <br />";
$src_tx=array(
    "code" => "txt",
    "desc" => "transifex trunk",
    "path" => "tx/translations/tortoisesvn.trunk-*",
    "listpath" => "trunk.actual");
$src_tx_br=array(
    "code" => "txb",
    "desc" => "transifex branch (unused?)",
    "path" => "tx/translations/tortoisesvn.branch-*",
    "listpath" => "branch.actual/1.6.x");
$src_trunk=array(
    "code" => "trunk",
    "desc" => "svn trunk",
    "path" => "trunk.actual");
$src_16x=array(
    "code" => "16x",
    "desc" => "1.6.x (old stable) branch",
    "path" => "branch.actual/1.6.x");
$src_tx17=array(
    "code" => "tx17",
    "desc" => "transifex branch (stable)",
    "path" => "tx/translations/tortoisesvn.*-1-7-x",
    "listpath" => "branch.actual/1.7.x");
$src_17x=array(
    "code" => "17x",
    "desc" => "1.7.x (stable) branch",
    "path" => "branch.actual/1.7.x");

$sources=array($src_trunk, $src_tx, $src_17x, $src_tx17, $src_16x, $src_tx_br);

$sourceDefault=$source;
if (!isset($source)) {
    $sourceDefault="trunk";
}
// validate source

// validate lang

// build lang lists
echo "<ul>";
foreach ($sources as &$src) {
    $src_code=$src["code"];
    $src_desc=$src["desc"];
    $src_path=$src["path"];
    $src_path2=isset($src["listpath"]) ? $src["listpath"] : "";
    $sourceList = new TSource;
    $sourceList->BuildList($dirBase . $src_path, $src_path2 ? $dirBase.$src_path2 : "");
    $src["srcs"] = $sourceList;

    $count = count($sourceList->list);

    if ($sourceDefault!=$src_code) {
        echo '<li><a href="/?s='.$src_code.'&amp;l=' . (isset($lang) ? $lang : "") .'">'.$src_desc.'</a> <sup>('.$count.')</sup> </li>';
    } else {
        echo '<li>'.$src_desc.' <sup>('.$count.')</sup> </li>';
    }
}
echo "</ul>";

$langlist=array();
$sourceTarget = $sourceDefault;
$langTarget = $lang;
if (isset($langTarget) && !$langTarget) {
    unset($sourceTarget);
}
if (isset($langTarget)) {
    unset($sourceTarget);
}
foreach ($sources as $src) {
    $src_code=$src["code"];
    if (!isset($sourceTarget) || $sourceTarget==$src_code) {
        $sourceList=$src["srcs"];
        foreach ($sourceList->list as $langData) {
            $code=$langData['Code'];
            if (!isset($langTarget) || $langTarget==$code) {
                $langlist[$code][$src_code]=$langData;
            }
        }
    }
}
ksort($langlist);


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
}

/*  $potGui=new TPo;
    $potGui->load("$dirLocation/Tortoise.pot", NULL);
    $potMerge=new TPo;
    $potMerge->load("$dirDoc/TortoiseMerge.pot", NULL);
    $potSvn=new TPo;
    $potSvn->load("$dirDoc/TortoiseSVN.pot", NULL);
    $potSvn=new TPo;
    $potSvn->load("$dirLocation/TortoiseDoc.pot", NULL);
    $pos=array();

    $data=array();//*/

/*  $potGui=new TPo;
    $potGui->load("$dirLocation/TortoiseUI.pot", NULL);
    $potDoc=new TPo;
    $potDoc->load("$dirLocation/TortoiseDoc.pot", NULL);//*/



    // iterate over lines
    unset($msgid);
    echo "<a name=\"TAB$lang\"></a>";
    echo '<table border="1"><thead><tr>
        <td rowspan="2"><acronym title="Native language name in English"><img src="'.$icons['language'].'" alt="" />Language</acronym></td>
        <td colspan="6"><img src="'.$icons['gui'].'" alt="" />GUI check</td>
        <td colspan="3"><img src="'.$icons['doc'].'" alt="" />DOC</td>
        <td rowspan="2"><img src='.$icons['authors'].' alt="" />Author(s)</td>
    </tr><tr>
        <td><img src="'.$icons['flag'].'" />Flag</td>
        <td><acronym title="Parameter test (Severity: High - may be harmfull)"><img src="'.$icons['error'].'" alt="" />PAR!!</acronym></td>
        <td><acronym title="Accelerator test (Severity: Medium - accessibility)"><img src="'.$icons['info'].'" alt="" />ACC!</acronym></td>
        <td><acronym title="Untranslated (Severity: Low - appearance)"><img src="'.$icons['new'].'" alt="" />UNT</acronym></td>
        <!--<td><acronym title="Fuzzy mark test (Severity: Low - appearance)"><img src="'.$icons['unknown'].'" alt="" />FUZ</acronym></td>-->
        <td><acronym title="Escaped chars (Severity: Low - appearance)"><img src="'.$icons['info'].'" alt="" />ESC</acronym></td>
        <td><img src="'.$icons['note'].'" alt="" />Note</td>
        <td><img src="'.$icons['doc'].'" title="Tortoise DOC" /></td>
        <td><img src="/images/32/tsvn.png" title="TortoiseSVN DOC" /></td>
        <td><img src="/images/32/tmerge.png" title="TortoiseMerge DOC" /></td>
    </tr></thead>
    <tbody>';

    function ExtractErrorCount($po, $name) {
        if (is_array($po)) {
            return $po[$name];
        } else if (is_a($po, "TPo")) {
            return $po->GetErrorCount($name);
        }
        echo "error ---";
        echo get_class($po);
        echo "---";
        return false;
    }

/*  function GetCheckResulForLanguage($po, $lang, $pot, $db, $group) {
        if (!is_a($po, "TPo")) {
            echo "Error".__FILE__.":".__LINE__."<br />\n";
            echo "is a ".get_class($po)."<br />\n";
            return false;
        }
        //unset($poTemp);
/*      if (isset($db) && isset($group) && $db!=NULL && $group!="") { // this is a hack ! - redesign !
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
        }//*/
//      $poTemp=new TPo;
//      $poTemp->Load($fileName, $lang);
//      $poTemp->AddPot($pot);
//      $poTemp->buildReport();
/*
        return $po;
    }//*/

    function PrintErrorCount($po, $name, $link="") {
        $errorCount=ExtractErrorCount($po, $name);
        $warningCount=0;
        if (is_array($errorCount)) {
            $warningCount=$errorCount('warnings');
            $errorCount=$errorCount('errors');
        }
        if ($errorCount===false || !isset($errorCount)) {
            echo "<td>$name?</td>\n";
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
        return;
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

    $rowClassIndex=0;
    $rowClasses=array("odd", "even");
    unset($countries);
    $countriesFile="trans_countries.inc";
    $countriesFile=$dir."/contrib/drupal-modules/translation-status/".$countriesFile;
    if (file_exists($countriesFile)) {
        include $countriesFile;
    }


    // build file list for all sources


    function findCoutriesParam($countries, $code, $index, $default) {
        $ret=$default;
        if (isset($countries[$code])) {
            return $countries[$code][$index];
        }
        return $ret;
    }


    // prefill variables used in loop
    $reportCodeList=array("par", "acc", "unt", /*"fuz", */"esc"/*, "spl"*/);
/*  if (!$langSelected && !$stable) { // this is a hack ! - redesign !
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

    $pos = array();
    function GetPoFile($filename) {
        global $pots;
        if (!isset($pots[$filename])) {
            $pots[$filename] = new TPo($filename, NULL);
        }
        //echo "PO:$filename<br/>\n";
        return $pots[$filename];
    }

    $pots = array();
    foreach ($langlist as $langs) {
        foreach ($langs as $src=>$lang) {
            $state    = isset($lang['Enabled']) ? $lang['Enabled'] : NULL;
            $code     = $lang['Code'];
            $language = $lang['LangName'];
            $flag     = $lang['Flag'];
            //$flags    = $lang['Flags'];
            $author   = $lang['Credits'];

            $pos[$code]   = NULL;

            //echo "<pre>"; var_dump($lang); echo "<pre/>";
            $poUi     = GetPoFile($lang['PoUiFile']);
            //$poDoc    = isset($lang['PoDocFile']) ? GetPoFile($lang['PoDocFile']) : NULL;
            //$poTsvn   = isset($lang['PoTsvnFile']) ? GetPoFile($lang['PoTsvnFile']) : NULL;
            //$poMerge  = isset($lang['PoMergeFile']) ? GetPoFile($lang['PoMergeFile']) : NULL;
            $potUi    = isset($lang['PotUiFile']) ? GetPoFile($lang['PotUiFile']) : NULL;
            //$potDoc   = isset($lang['PotDocFile']) ? GetPoFile($lang['PotDocFile']) : NULL;
            //$potTsvn  = isset($lang['PotTsvnFile']) ? GetPoFile($lang['PotTsvnFile']) : NULL;
            //$potMerge = isset($lang['PotMergeFile']) ? GetPoFile($lang['PotMergeFile']) : NULL;

            $rowClass = $rowClasses[$rowClassIndex];
            $rowClassIndex = ($rowClassIndex+1)%count($rowClasses);

            echo "<tr class=\"$rowClass\">";
            echo "<td>".$language."</td>\n";
            $link="?s=$sourceDefault&amp;m=$m&amp;l=$code";
            $flagcode=$code;
            $imagesrc="http://tortoisesvn.net/flags/world.small/$flagcode.png";
            $imagesrc2=$flag;
            // todo: change fix sizes into class definition
            if (isset($imagesrc2)) {
                $imagesrc2="/images/flags/$imagesrc2.png";
                echo "<td><a href=\"$link#TAB$code\"><img src=\"$imagesrc2\" alt=\"$code\" height=\"48\" width=\"48\" /></a></td>\n";
            } else {
                echo "<td><a href=\"$link#TAB$code\"><img src=\"$imagesrc\" alt=\"$code\" height=\"24\" width=\"36\" /></a></td>\n";
            }

            //$po = GetCheckResulForLanguage($poUi, $code, $potUi, $dbLink1, $m);
            $po = $poUi;
            $po->AddPot($potUi);
            $po->BuildReport();
            $errorCount=0;
            foreach ($reportCodeList as $reportCode) {
                $errorCount+=PrintErrorCount($po, $reportCode, $link);
            }

            // note
            if ($state===false) {
                echo '<td><img src="'.$icons['disabled'],'" alt="Disabled" title="Disabled"/></td>';
            } else if(!$errorCount) {
                echo '<td><img src="'.$icons['ok'],'" alt="o.k." title="OK"/></td>';
            } else {
                echo "<td />";
            }

            if (isset($filenameDoc)) {
                $statDoc=GetDocStatus($filenameDoc, $code, $potDoc, $dbLink2, 'd');
            } else {
                $statDoc="-";
            }
            if ($statDoc=="-") {
            } else {
                if ($statDocTsvn=="OK") {
                    $statDocTsvn='<img src="'.$icons['ok'].'" alt="o.k." title="OK"/>';
                }
                $statDocTsvn='<a href="/?stable='.$stable.'&amp;l='.$code.'&amp;m=s">'.$statDocTsvn.'</a>';
            }
            echo "<td>$statDoc</td>\n";

            if (isset($potSvn)) {
                $statDocTsvn=GetDocStatus($filenameTsvn, $code, $potSvn, $dbLink2, 't');
            } else {
                $statDocTsvn="-";
            }
            if ($statDocTsvn=="-") {
            } else {
                if ($statDocTsvn=="OK") {
                    $statDocTsvn='<img src="'.$icons['ok'].'" alt="o.k." title="OK"/>';
                }
                $statDocTsvn='<a href="/?stable='.$stable.'&amp;l='.$code.'&amp;m=s">'.$statDocTsvn.'</a>';
            }
            echo "<td>$statDocTsvn</td>\n";//*/

            if (isset($potMerge)) {
                $statDocMerge=GetDocStatus($filenameMerge, $code, $potMerge, $dbLink2, 'm');
            } else {
                $statDocMerge="-";
            }
            if ($statDocMerge=="-") {
            } else {
                if ($statDocMerge=="OK") {
                    $statDocMerge='<img src="'.$icons['ok'].'" alt="o.k." title="OK"/>';
                }
                $statDocMerge='<a href="/?stable='.$stable.'&amp;l='.$code.'&amp;m=m">'.$statDocMerge.'</a>';
            }
            echo "<td>$statDocMerge</td>\n";//*/

                    echo "<td>".$author."</td>";
                    echo "</tr>\n";
                    $pos[$code]=$po;
                    flush();
//              }
        }
//      break;
    }
    echo '</tbody></table>';
    exit;


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
#       $this->Name=$Name;
#       $this->Files=$Files;
#       $this->SubDirs=$SubDirs;
#       $this->Location=$Loc;

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
