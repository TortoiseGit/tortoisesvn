<?php

	function PrintTableOld($header, $data){
		// Printing results in HTML
		echo "\t<table border=1 cellpadding=0 cellspacing=0 frame=void><thead><tr>\n";
		foreach ($header as $col_name) {
			echo "\t\t<td bgcolor=\"#666600\"><b>$col_name</b></td>\n";
		}
		echo "</tr></thead><tbody>\n";

		$bgcolors[0]="#ffff99";
		$bgcolors[1]="#b3b300";
		$linenum=0;
		foreach ($data as $line) {
			$bgcolor=$bgcolors[$linenum%2];
			$linenum++;
			echo "\t<tr>\n";
			foreach ($line as $col_value) {
				unset($url);
				if (is_array($col_value)) {
					if (isset($col_value['text'])) {
						$text=$col_value['text'];
					} else {
						$text=$col_value[0];
					}
					$url=$col_value['url'];
				} else {
					$text=$col_value;
				}
				if (isset($url)) {
					echo "\t\t<td BGCOLOR=\"$bgcolor\"><a href=\"$url\">$text</a></td>\n";
				} else {
					echo "\t\t<td BGCOLOR=\"$bgcolor\">$text</td>\n";
				}
			}
			echo "\t</tr>\n";
		}
		echo "</tbody></table>\n";
	}


	function PrintTable($header, $data, $caption=NULL){
		// Printing results in HTML
		echo "\t<table border=\"1\" cellpadding=\"0\" cellspacing=\"0\" frame=\"void\">";
		if ($caption!=NULL) {
			echo "<caption>$caption</caption>";
		}
		echo "<thead><tr>\n";
		foreach ($header as $col_name) {
#			echo "\t\t<td bgcolor=\"#666600\"><b>$col_name</b></td>\n";
			echo "\t\t<td><b>$col_name</b></td>\n";
		}
		echo "</tr></thead><tbody>\n";

		$lineClasses[0]="tl1";
		$lineClasses[1]="tl2";
		$lineClassIndex=0;
		$lineNum=0;
		foreach ($data as $line) {
			$lineClass=$lineClasses[$lineClassIndex++];
			if ($lineClassIndex>=count($lineClasses)) {
				$lineClassIndex=0;
			}
			$lineNum++;
			echo "\t<tr>\n";
			foreach ($line as $col_value) {
				echo "\t\t<td class=\"$lineClass\">";
				$column=$col_value;
				if (is_array($column)) {
				} else {
					$column=array('text' => $column);
				}
				$alt=$column['alt'];
				$acronym=$column['acronym'];
				$class=$column['class'];
				$image=$column['image'];
				$text=$column['text'];
				if (isset($text)) {
					$text=str_replace(array("&"), array("&amp;"), $text);
				}
				$url=$column['url'];
				if (isset($acronym)){
					echo "<acronym title=\"$acronym\">";
				}
				if (isset($url)){
					echo "<a href=\"$url\" title=\"$alt\">";
				}
				if (isset($image)){
					echo "<img src=\"$image\" alt=\"$alt\" class=\"$class\" />";
				}
				if (isset($text)){
					echo $text;
				}
				if (isset($url)) {
					echo "</a>";
				}
				if (isset($acronym)){
					echo "</acronym>";
				}
				echo "</td>\n";
			}
			echo "\t</tr>\n";
		}
		echo "</tbody></table>\n";
	}


	
//php?>