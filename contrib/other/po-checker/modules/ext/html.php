<?php
define ("NEW_LINE", "\r\n");
define ("NEW_LINE_TAB", "\r\n\t");

define ("HTML_STRIPT_JAVASCRIPT", "");

define ("HTML_STYLE_CSS", "text/css");

function HtmlElement($Element, $Args=NULL) {
	echo "<$Element";
	if (is_array($Args)) {
		foreach ($Args as $key=>$value) {
			if ($key!="/") {
				echo " $key=\"$value\"";
			}
		}
	}
	echo ">";
}
//----------------------------------------------------------------------------

function HtmlHeaderElement($HtmlType, $HtmlHeaderElementType, $HtmlHeaderElement) {
	if (count($HtmlHeaderElement)>1 || is_array($HtmlHeaderElement) || !isset($HtmlHeaderElement["/"])) {
		echo NEW_LINE_TAB;
		HtmlElement($HtmlHeaderElementType, $HtmlHeaderElement);
		echo $HtmlHeaderElement["/"]."</$HtmlHeaderElementType>";
	}
}
//----------------------------------------------------------------------------

function HtmlHeaderElements($HtmlType, $HtmlHeaderElementType, $HtmlHeaderElements) {
	if (!is_array($HtmlHeaderElements)) {
		return;
	}
	foreach ($HtmlHeaderElements as $HtmlHeaderElement) {
		HtmlHeaderElement($HtmlType, $HtmlHeaderElementType, $HtmlHeaderElement);
	}
}
//----------------------------------------------------------------------------

function HtmlHeader($HtmlType, $HtmlHeader=NULL, $HtmlBody=NULL) {
	switch ($HtmlType) {
	 case "xhtml10s":
		//header('content-type: application/xhtml+xml');
		echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">';
		echo NEW_LINE;
		echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">';
		echo NEW_LINE;
		break;
	 case "xhtml10t":
//		header("content-type: application/xhtml+xml");
		echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">';
		echo NEW_LINE;
		echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">';
		echo NEW_LINE;
		break;
	 case "xhtml11":
//		header("content-type: application/xhtml+xml");
		echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">';
		echo NEW_LINE;
		echo '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">';
		echo NEW_LINE;
		break;
	 default:
		echo "UNKNOWN HTML CODING";
	}

	echo '<head>';
	echo "\r\n\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />";
	echo "\r\n\t<title>".$HtmlHeader["title"]."</title>";
	echo NEW_LINE_TAB;
	echo '<link rel="shortcut icon" href="favicon.ico" />';

	HtmlHeaderElements($HtmlType, "style", $HtmlHeader["style"]);
	HtmlHeaderElements($HtmlType, "script", $HtmlHeader["script"]);
/*	if (isset($css)) {
		echo "<link rel=\"stylesheet\" href=\"$css\" type=\"text/css\" />";
	}//*/
	echo NEW_LINE;
	echo '</head>';
	echo NEW_LINE;
	echo NEW_LINE;
	HtmlElement("body", $HtmlBody);
}
//----------------------------------------------------------------------------

function HtmlFooter($HtmlType) {
	$NEW_LINE="\r\n";

	echo "</body>";
	echo $NEW_LINE;
	echo "</html>";
}
php?>