<?php // $Id$

//include_once "CSV";

function debug($text) {
    //echo $text;
}

class TSource {
    // propery declaration
    /**
        array of {
         'Code' : sk
         'Enabled'? : true
         'Lang'? : Slovak
         'Flag'? :
         'Flags'? : 001
         'LangName'? :
         'Credits'? :
         'WixLang'? :
         'PoUiFile' :
         'PoDocFile' :
         'PoTsvnFile'? :
         'PoMergeFile'? :
        }
    */
    public $list = array();
    public $baseDir = "";
    public $infoDir = "";

    // method declaration
    public function BuildList($baseDir, $infoDir=NULL) {
        debug("Building lang list for $baseDir ($infoDir)<br />");

        // clear list
        $this->list = array();
        unset($this->potUi);
        unset($this->potDoc);
        unset($this->potTsvn);
        unset($this->potMerge);

        // set parameters
        $this->baseDir = $baseDir;
        if ($infoDir==NULL) {
            $this->infoDir = $baseDir;
        } else {
            $this->infoDir = $infoDir;
        }
        if (is_dir($this->baseDir)) {
            $this->baseDir .= '/';
        }
        if (is_dir($this->infoDir)) {
            $this->infoDir .= '/';
        }
        debug("Building lang list for ".$this->baseDir." (".$this->infoDir.")<br />");

        // add files based on rules
        $this->AddLangsFromDir(); // - find all existing files; multiple layouts
        //$this->AddLangFromTx();
        //$this->AddLangsFromInc(); //r13328 - r21748
        $this->AddLangsFromTxt(); // r1302 - today
        $this->Clean();
        $this->SetFlagNames();
        $this->SetPotFiles();
    }
    //---------------------------------------------------------------------------


    /** */
    protected function AddLangsFromDir() {
        if (is_dir($this->baseDir)) {
            $this->AddLangsFromDir_classic(); //
            $this->AddLangsFromDir_svntx(); // tx supported - simple, and with LC_MESSSAGE
        } else {
            $this->AddLangsFromDir_tx(); // tx layout
        }
    }
    //---------------------------------------------------------------------------


    /** */
    protected function AddLangsFromDir_classic() {
        $dirPath = $this->baseDir . "Languages/";

        debug("Adding files from " . $dirPath ."<br />");

        if (!is_dir($dirPath)) {
            debug($dirPath . "is not valid dir");
            return false;
        }
        $dirPathDoc = $this->baseDir . "doc/po/";
        if (($dirHandle = opendir($dirPath))==false) {
            return false;
        }
        while (($file = readdir($dirHandle)) !== false) {
            if ($file[0]=='.') {
                continue;
            }
            $fullFileName = $dirPath.$file;
            if (substr($file, -3)==".po" && is_file($fullFileName)) {
                //debug($fullFileName."<br/>");
                $lang = array();
                $lang['PoUiFile'] = $fullFileName;
                $code=substr($file, 9, -3);

                if (isset($dirPathDoc) && file_exists($dirPathDoc."TortoiseDoc_$code.po")) {
                    $lang['PoDocFile'] = $dirPathDoc."TortoiseDoc_$code.po";
                }
                if (isset($dirPathDoc) && file_exists($dirPathDoc."TortoiseSVN_$code.po")) {
                    $lang['PoTsvnFile'] = $dirPathDoc."TortoiseSVN_$code.po";
                }
                if (isset($dirPathDoc) && file_exists($dirPathDoc."TortoiseMerge_$code.po")) {
                    $lang['PoMergeFile'] = $dirPathDoc."TortoiseMerge_$code.po";
                }

                if (count($lang)>0) {
                    $lang['Code'] = $code;
                    $this->UpdateLang($lang);
                }
            }
        }//*/
        return true;
    }
    //---------------------------------------------------------------------------


    /** */
    protected function AddLangsFromDir_svntx() {
        $dirPath = $this->baseDir . "Languages/";

        debug("Adding files from " . $dirPath ."<br />");

        if (!is_dir($dirPath)) {
            debug($dirPath . "is not valid dir");
            return false;
        }
        if (is_file($dirPath . "TortoiseDoc.pot")) {
            $this->potDoc = $dirPath . "TortoiseDoc.pot";
        }
        if (($dirHandle = opendir($dirPath))==false) {
            return false;
        }
        while (($file = readdir($dirHandle)) !== false) {
            if ($file[0]=='.') {
                continue;
            }
            $fullFileName = $dirPath.$file;
            if (is_dir($fullFileName)) {
                $lang = array();
                if (file_exists($fullFileName."/TortoiseDoc.po")) {
                    $lang['PoDocFile'] = $fullFileName."/TortoiseDoc.po";
                } else if (file_exists($fullFileName."/LC_MESSAGES/TortoiseDoc.po")) {
                    $lang['PoDocFile'] = $fullFileName."/LC_MESSAGES/TortoiseDoc.po";
                }
                if (file_exists($fullFileName."/TortoiseUI.po")) {
                    $lang['PoUiFile'] = $fullFileName."/TortoiseUI.po";
                } else if (file_exists($fullFileName."/LC_MESSAGES/TortoiseUI.po")) {
                    $lang['PoUiFile'] = $fullFileName."/LC_MESSAGES/TortoiseUI.po";
                }
                if (count($lang)>0) {
                    $lang['Code'] = $file;
                    $this->UpdateLang($lang);
                }
            }
        }
        return true;
    }
    //---------------------------------------------------------------------------


    /** */
    protected function AddLangsFromDir_tx() {
        debug("Adding files from " . $this->baseDir ."<br />");

        $dirPathGui = str_replace("*", "gui", $this->baseDir) . "/";
        if (!is_dir($dirPathGui)) {
            debug($dirPathGui." NotFound<br/>");
            return false;
        }
        $dirPathDoc = str_replace("*", "manual", $this->baseDir) . "/";
        if (!is_dir($dirPathDoc)) {
            unset($dirPathDoc);
        }
        $dirPathTsvn = str_replace("*", "manual-tsvn", $this->baseDir) . "/";
        if (!is_dir($dirPathTsvn)) {
            unset($dirPathTsvn);
        }
        $dirPathMerge = str_replace("*", "manual-tmerge", $this->baseDir) . "/";
        if (!is_dir($dirPathMerge)) {
            unset($dirPathMerge);
        }

        if (($dirHandle = opendir($dirPathGui))==false) {
            return false;
        }
        while (($file = readdir($dirHandle)) !== false) {
            if ($file[0]=='.') {
                continue;
            }
            $fullFileName = $dirPathGui.$file;
            if (is_file($fullFileName)) {
                $lang = array();
                $lang['PoUiFile'] = $fullFileName;
                if (isset($dirPathDoc) && file_exists($dirPathDoc.$file)) {
                    $lang['PoDocFile'] = $dirPathDoc.$file;
                }
                if (isset($dirPathTsvn) && file_exists($dirPathTsvn.$file)) {
                    $lang['PoTsvnFile'] = $dirPathTsvn.$file;
                }
                if (isset($dirPathMerge) && file_exists($dirPathMerge.$file)) {
                    $lang['PoMergeFile'] = $dirPathMerge.$file;
                }
                if (count($lang)>0 && substr($file, -3)==".po") {
                    $lang['Code'] = substr($file, 0, -3);
                    $this->UpdateLang($lang);
                }
            }
        }//*/
        return true;
    }
    //---------------------------------------------------------------------------


    /** */
    protected function AddLangsFromInc() {

    }
    //---------------------------------------------------------------------------


    /**
        Loads lang list from Languages.txt file and fill its parameters
        Note: can not fill po files
    */
    protected function AddLangsFromTxt() {
        $languagelistFileName = $this->infoDir . 'Languages/Languages.txt'; // r1434 - today
        if (!file_exists($languagelistFileName)) {
            debug("File $languagelistFileName does not exixts!<br/>");
            $languagelistFileName = $this->infoDir . 'src/Languages/Languages.txt'; // r1302 - r1433
        }
        if (!file_exists($languagelistFileName)) {
            debug("File $languagelistFileName does not exixts!<br/>");
            debug("No file to process found !<br/>");
            return false;
        }
        debug("Processing file $languagelistFileName.<br/>");
        // load language.txt
        $csv = new parseCSV();
        $csv->delimiter = ";";
        $csv->parse($languagelistFileName);


        foreach ($csv->data as $csvLine) {
            if (!isset($csvLine) || !is_array($csvLine) || count($csvLine)<1) {
                continue;
            }
            $indexedArray=array_values($csvLine);
            if ($indexedArray[0][0]=='#') {
                // to do process if everithing else match - aka commented out lang
                continue;
            }
            $ret=array();
            switch (count($csvLine)) {
             case 6:
                /*echo "<br /><pre>";
                var_dump($indexedArray);
                echo "</pre>";
                /*if (preg_match("/^[-01]/", $indexedArray[0])) {
                    $ret['Enabled']=$indexedArray[0];
                    $ret['Code']=$indexedArray[1];
                    //$ret['Lang']=$indexedArray[2];
                    $ret['Flags']=$indexedArray[3];
                    $ret['LangName']=$indexedArray[4];
                    $ret['Credits']=$indexedArray[5];
                } else {
                    $ret['Code']=$indexedArray[0];
                    //$ret['Lang']=$indexedArray[1];
                    $ret['Enabled']=$indexedArray[2];
                    //$ret['Flags']=$indexedArray[3];
                    $ret['LangName']=$indexedArray[4];
                    //$ret['Credits']=$indexedArray[5];
                    // Credits should be loaded from other file ... (vars)
                }//*/

                // 21716 -
                $ret['Code']=trim($indexedArray[0]);
                $ret['WixLang']=trim($indexedArray[1]);
                //$ret['LCID']=trim($indexedArray[2]);
                $ret['SC']=trim($indexedArray[3]);
                $ret['LangName']=trim($indexedArray[4]);
                $ret['Credits']=trim($indexedArray[5]);
                break;

             case 7:
                $ret['Enabled']=trim($indexedArray[0]);
                $ret['Code']=trim($indexedArray[1]);
                $ret['WixLang']=trim($indexedArray[2]);
                $ret['Lang']=trim($indexedArray[3]);
                $ret['Flags']=trim($indexedArray[4]);
                $ret['LangName']=trim($indexedArray[5]);
                $ret['Credits']=trim($indexedArray[6]);
            }

            if (count($ret)>0) {
                $this->UpdateLang($ret);
            }
        }
    }
    //---------------------------------------------------------------------------


    /** */
    protected function Clean() {
        foreach ($this->list as $key => $lang) {
            if (isset($lang['PoUiFile'])) {
                continue;
            }
            /*if (isset($lang['PoDocFile'])) {
                continue;
            }
            if (isset($lang['PoTsvnFile'])) {
                continue;
            }
            if (isset($lang['PoMergeFile'])) {
                continue;
            }//*/
            unset($this->list[$key]);
        }
    }
    //---------------------------------------------------------------------------


    /** */
    protected function SetFlagNames() {
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
                "sr" => "Serbia(Yugoslavia)",
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

        foreach ($this->list as &$row) {
            if (isset($row['Lang']) && isset($langToFlag[$row['Lang']])) {
                $row['Flag']=$langToFlag[$row['Lang']];
            }
            if (isset($row['Code']) && isset($langToFlag[$row['Code']])) {
                $row['Flag']=$langToFlag[$row['Code']];
            }

        }
    }
    //---------------------------------------------------------------------------


    /** */
    protected function SetPotFiles() {
        $this->potUi = "/var/www/po-checker/data/trunk.actual/Languages/TortoiseUI.pot";

        if (isset($this->potUi)) {
            foreach ($this->list as &$row) {
                $row['PotUiFile'] = $this->potUi;
            }
        }
        if (isset($this->potDoc)) {
            foreach ($this->list as &$row) {
                $row['PotDocFile'] = $this->potDoc;
            }
        }
        if (isset($this->potTsvn)) {
            foreach ($this->list as &$row) {
                $row['PotTsvnFile'] = $this->potTsvn;
            }
        }
        if (isset($this->potMerge)) {
            foreach ($this->list as &$row) {
                $row['PotMergeFile'] = $this->potMerge;
            }
        }
    }
    //---------------------------------------------------------------------------

    /** */
    protected function UpdateLang($langData) {
        if (!isset($langData['Code'])) {
            return false;
        }
        // find record with lang
        foreach ($this->list as &$value) {
            if ($value['Code'] == $langData['Code']) {
                foreach ($langData as $paramName => $paramValue) {
                    $value[$paramName] = $paramValue;
                }
                return true;
            }
        }
        // not found add
        $this->list[] = $langData;
        return true;
    }
    //---------------------------------------------------------------------------
}
