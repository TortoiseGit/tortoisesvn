<?php

function Unicode2Utf8($text) {
	$char=0;
	$res="";
	for ($i=0; $i<strlen($text); $i+=2){
		$char=$text[$i+1];
		$res.=$char;
	}
	return $res;
}

class rc{
	protected $objects=array();
	protected $data=array();

	function Load($rcFileName){
		echo "<i>Loading $rcFileName</i><br />\n";
		$this->objects=array();
		$this->lines=array();
		// open_file & load lines
		$linesLoaded=file($rcFileName);
		// iterate over lines
		$lastLineType=RC_NONE;
		$objectWrk=NULL;
		$lineNum=0;
		$depth=0;
		$deepText=" ";
		foreach ($linesLoaded as $line) {
			if ($lineNum==0 && ord($line[0])==0xff && ord($line[1])==0xfe) {
//				echo "UNICODE";
			}
#			echo $line."<br>";
			$line=Unicode2Utf8($line);
#			echo $line."<br>";
			$lineNum++;
#			if ($lineNum>100) break;
#			if ($lineNum>10) {
#				break;
#			}
			$lineType=RC_UNKNOWN;
			// remove \n from end
			preg_match_all("/^([^\r\n]*)[\r\n]*$/", $line, $result, PREG_PATTERN_ORDER);
			$line=$result[1][0];
			if ($line==""){
				continue;
			}
			// is it a comment ?
			preg_match_all("/^[ \t]*(\/\/|\"|#|[\r\n]*$)/", $line, $result, PREG_PATTERN_ORDER);
			$commentType=$result[1][0];
			if ($commentType|=""){
				continue;
			}
			// command BEGIN / END
			preg_match_all("/^[ \t]*(BEGIN|END)/",$line,$result, PREG_PATTERN_ORDER);
			$command=$result[1][0];
			if ($command!=""){
#				echo "<br />C: $command";
				switch($command){
				 case "BEGIN":
				 	$lineType=RC_BEGIN;
					$groupsNum++;
					$depth++;
					$deepText=str_pad("",($depth*2+1)*6,"&nbsp;",STR_PAD_LEFT);
					break;
				 case "END":
					$lineType=RC_END;
					$depth--;
					$deepText=str_pad("",($depth*2+1)*6,"&nbsp;",STR_PAD_LEFT);
				 	break;
				}
				continue;
			}
			// subobject deefinition
			preg_match_all("/^[ \t]*((BOTTOM|LEFT|RIGHT|TOP)MARGIN|CAPTION|(COMBO|GROUP|LIST)BOX|CONTROL|(C|EDIT|L)TEXT|((DEF)?PUSH)?BUTTON|(EX)?STYLE|FONT|GUIDELINES|LANGUAGE|MENU(ITEM)?|POPUP|SEPARATOR|STRINGTABLE|VERTGUIDE)/",$line,$result, PREG_PATTERN_ORDER);
			$parameter=$result[1][0];
			if ($parameter!=""){
				if ($objectStrings===NULL){
					continue;
				}
				preg_match_all("/^[^\"]*\"([^\"]*)\"/",$line,$result, PREG_PATTERN_ORDER);
				$data=$result[1][0];
				if ($data!=""){
#					echo "<br />P:$deepText$parameter:$data";
					$objectStrings[]=$data;
					$objectParams[$parameter]=$data;
					$this->texts[$data]=true;
#					echo " - ".count($objectWrk["STRINGS"])."/".count($this->objects[count($this->objects)-1]["STRINGS"]);
				}
				continue;
			}
			// object definition
			preg_match_all("/^[ \t]*[[:alnum:]_]*[ \t]*(ACCELERATORS|AVI|BITMAP|CURSOR|DIALOGEX|ICON|TOOLBAR|\"[^\"]*\")/",$line,$result, PREG_PATTERN_ORDER);
			$object=$result[1][0];
			if ($object!="") {
				if ($depth==0) {
					$this->objects[]=array("TYPE"=>$object, "STRINGS"=>array(), "PARAMETERS"=>array());
					$objectStrings=&$this->objects[count($this->objects)-1]["STRINGS"];
					$objectParams=&$this->objects[count($this->objects)-1]["PARAMETERS"];
				}
#				echo "<br />O:$deepText$object";
				continue;
			}
#			echo "<br /><br />L:".$lineNum." $line";
			if ($lineType!=RC_UNKNOWN){
#				echo "<br /><br />L:<b>".htmlspecialchars($line)."</b>";
#				echo "<br />C:<i>".$command."</i>";
#				echo "<br />D:<i><b>".$data."</b></i>";
			}
#			echo "<br />".$lineType;
			switch($lineType){
			 case PO_MSGID:
			 	if ($lineRep){
					$msgid.=$data;
					$this->lines[]=array($lastLineType=PO_DATA, $line);
				}else{
					$msgid=$data;
					$this->lines[]=array($lastLineType=PO_MSGID, $line);
				}
#				echo "<br />ID:".$poFileName.": ".$msgid;
			  break;
			 case PO_MSGSTR:
			 	if ($lineRep){
					$msgstr.=$data;
					$this->lines[]=array($lastLineType=PO_DATA, $line);
				}else{
					$msgstr=$data;
					$this->lines[]=array($lastLineType=PO_MSGSTR, $line);
				}
				$this->dictionary[$msgid]=$msgstr;
#				echo "<br />STR:".$this->dictionary[$msgid];
			  break;
			}
			$lastLineType=$lineType;
		}
		#echo "<br />Lines:".$lineNum."<br />texts:".count($this->texts)."<br />depth: $depth<br />Groups: $groupsNum<br /><br />";
	}

	function Translate($po){
		echo "translating ...<br />\n";
		if (is_string($po)){
			$poObj=new po; 
			$poObj->Load($po);
			$po=$poObj;
		}
			
		if (get_class($po)=="po"){
		  	foreach ($this->objects as $objKey=>$value) {
				foreach($value["STRINGS"] as $key=>$string) {
					$translation=$po->Translate($string);
					if ($translation!="") {
						$this->objects[$objKey]["STRINGS"][$key]=$translation;
					}
				}
			}
		} else {
			echo "Wrong parameter to translate<br />";
		}
		echo "done<br />\n";
	}

	function Report() {
		//acc self check
		$tableAcc=new Table;
		echo "<b>Accs:</b><br />";
		$line=0;
		foreach ($this->objects as $value) {
			$used=array();
			foreach ($value["STRINGS"] as $key=>$string) {
				preg_match_all("/&(.)/", $string, $result, PREG_PATTERN_ORDER);
				$acc=strtoupper($result[1][0]);
				if ($acc!=""){
					$used[$acc][]=$value;
				}
			}
			$overlaped=false;
			foreach ($used as $key=>$strings){
				if (count($strings)>1){
					$overlaped=true;
				}
			}
			if ($overlaped){
				echo "<i>CAPTION: \"".$value["PARAMETERS"]["CAPTION"]."\"</i>";
				foreach($value["STRINGS"] as $key=>$string){
					preg_match_all("/&(.)/u", $string, $result, PREG_PATTERN_ORDER);
					$acc=strtoupper($result[1][0]);
					$color="black";
					if (count($used[$acc])>1){
						$color="red";
					}
					if ($acc!=""){
						echo "\n<br /><font color=\"$color\"> A: $acc S: ".htmlspecialchars($string)."</font>";
					}
				}
				echo("\n<hr />");
			}
		}
		echo "";
		echo $line."<br />";
	}
}

//php?>
