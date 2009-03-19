<?php
function PrintArray($array) {
	if (is_array($array)) {
		echo "Array:";
		echo "\t<table border=\"1\" cellpadding=\"0\" cellspacing=\"0\" frame=\"void\"><tr>\n";
		foreach ($array as $key => $value) {
			if (is_array($value)) {
				echo "<tr><td>".$key."</td><td>";
				PrintArray($value);
				echo "</td></tr>\n";
			} else {
				echo "<tr><td>".$key."</td><td>".$value."</td></tr>\n";
			}
		}
		echo "</table>\n";
	} else {
		echo $array;
	}
}
//----------------------------------------------------------------------------

function PrintArrayT($array) {
	if (is_array($array)) {
		$lineData="";
		$lineHead="";
		foreach ($array as $key => $value) {
			if (false && is_array($value)) {
				echo "<tr><td>".$key."</td><td>";
				PrintArray($value);
				echo "</td></tr>\n";
			} else {
				$lineHead .= "<th>$key</th>";
				$lineData .= "<td>$value</td>";
			}
		}
		echo "\tArray:<table border=\"1\" cellpadding=\"0\" cellspacing=\"0\" frame=\"void\"><thead>$lineHead</thead><tr>$lineData</tr></table>\n";
	} else {
		PrintArray($array);
	}
}
//----------------------------------------------------------------------------

function CsvLineToArray($line) {
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
	return $array;
}
//----------------------------------------------------------------------------

function AddToQuery($query, $name, $value) {
	if (!isset($value) || $value=="") {
		return $query;
	}
	if (strpos($query, "=")) {
		$delimeter=",";
	} else {
		$delimeter="";
	}
	return $query.$delimeter." $name='$value'";
}
//----------------------------------------------------------------------------

function ExtractNumber($value) {
	$value = str_replace("\xc2\xa0", "", $value);
//	$value = str_replace(" ", "", $value);
	preg_match("/^-?[\s\d,\.]* /", $value, $matches);
	return trim(str_replace(",", ".", $matches[0]));
}
//----------------------------------------------------------------------------

function ExtractInvoiceNumber($value) {
//	$id=substr($id, 7, 2) . substr($id, 0, 4);
//	$id+=0;
	preg_match("/([0-9]{4}) \/ ([0-9]{2})/", $value, $matches);
	if ($matches[0]!="" && $matches[1]!="" && $matches[2]!="") {
		$value=$matches[2].$matches[1];
		return $value+0;
	}
	return $value;
}
//----------------------------------------------------------------------------

function ExtractDate($value) {
	preg_match("/([0-3]?[0-9])\.([0-1]?[0-9])\.([0-9]*)/", $value, $matches);
	if ($matches[0]!="" && $matches[1]!="" && $matches[2]!="" && $matches[3]!="") {
		return $matches[3]."-".$matches[2]."-".$matches[1];
	}
	return $value;
}
//----------------------------------------------------------------------------

function ValueOrNull($value) {
	if (!isset($value) || $value=="") {
		return "NULL";
	}
	return $value;
}
//----------------------------------------------------------------------------

php?>