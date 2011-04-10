<?php
require_once "Table.php";

class Csv extends Table {
	public $fieldDelimeter;
	public $textDelimeter;

	function Csv() {
		$this->fieldDelimeter=";";
		$this->textDelimeter="\"";
	}

	function Load($fileName) {
	$regexp = "/^(,|\\\"[^\\\"]*\\\",?|[0-9]*,|[0-3]?[0-9]\.[0-1]?[0-9]\.200[7-9],?)/";
	$i = 0;
	$array=array();
	while (count($line)) {
		$result = preg_match($regexp, $line, $matches); // find element
		if (!$result) break;
		$match=$matches[0];
		if (strlen($match)>2) {
			$res = preg_match("/\\\"(.*)\\\",?/", $match, $matches); // get data
			if (!isset($matches[1])) {
				$match=str_replace(",", "", $match);
				$array[$i]=$match;
			} else {
				$array[$i]=trim($matches[1]);
			}
		} else {
			$array[$i]="";
		}
		if ($i++>100) break; // limit elements - no endless loop
		$line = preg_replace($regexp, "", $line); // remove element
	}
	return $array;	}

	private function OutputLine($row) {
		$i=0;
		foreach ($row as $cell) {
			if ($i++) {
				echo $this->fieldDelimeter;
			}
			if (isset($cell) && $cell!=="") {
				echo $cell;
			}
		}
		echo "\x0d\x0a";
	}

	public function Output() {
		$this->OutputLine($this->header);
		foreach ($this->data as $data) {
			$this->OutputLine($data);
		}
	}
}
//php?>
