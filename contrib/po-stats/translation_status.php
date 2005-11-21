<!--break-->
<?php
// index.php
//
// Main page.  Lists all the translations

//include("trans_data.inc");
//include("trans_countries.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_data.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_countries.inc");

$vars['release']="1.2.6";
$vars['reposurl']="http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/Languages/";
//$vars['downloadurl1']="http://download.berlios.de/tortoisesvn/";
$vars['downloadurl1']="http://prdownloads.sourceforge.net/tortoisesvn/";
$vars['downloadurl2']="?download";
$basename="Tortoise";
$template=$basename.".pot";
$vars['flagpath']="/flags/world.small/";

function print_header($vars)
{
?>

<div class="content">
<h2>Translations (in Revision <?php echo $vars['wcrev']; ?>)</h2>

<p>
This page is informing you about the translation status of the current development version of TortoiseSVN, which is always ahead of the latest official release. The statistics are calculated for the HEAD revision and updated nightly. The last update was run at <b><?php echo $vars['update']; ?></b>.
</p>

<p>
The language pack installers can be downloaded from this page as well. The installers are always built for the last official release <b>(<?php echo $vars['release'] ?>)</b>. 
</p>

<p>
If you want to download the po file from the repository, either use <strong>guest/guest</strong> or your tigris.org user ID. 
</p>

<?php
}

function print_footer($vars)
{
$dlfile = $vars['downloadurl1']."Language_".$vars['release'].".zip".$vars['downloadurl2'];

?>
<p>
<img src="files/translated.png" alt="translated" title="translated" width="32" height="16"/> Translated <img src="files/missingaccelerator.png" alt="missing accelerator keys" title="missing accelerator keys" width="32" height="16"/> Missing accelerator keys <img src="files/fuzzy.png" alt="fuzzy" title="fuzzy" width="32" height="16"/> Fuzzy <img src="files/untranslated.png" alt="untranslated" title="untranslated" width="32" height="16" /> Untranslated
</p>
<p>
Translations were made by many people, you find them on  the list of <a href="http://tortoisesvn.tigris.org/contributors.html">contributors</a>.
</p>

</div>

<?php
}

function print_table_header($name, $summary, $vars)
{
?>
<h2><?php echo $summary ?></h2>
<div class="table">
<table class="translations" summary="<?php echo $summary ?>">
<tr>
<th class="lang">Nr.</th>
<th class="lang">Download Installer (<?php echo $vars['release'] ?>)</th>
<th class="lang">ISO code</th>
<th colspan="2" class="trans">(<?php echo $vars['total'] ?>) Complete</th>
<th class="graph">Graph</th>
<th class="download">Download .po file</th>
<th class="download">Last update</th>
</tr>
<?php
}

function print_table_footer()
{
?>
</table>
</div>
<div style="clear:both">&nbsp;<br/></div>
<?php
}

function print_blank_stat($i, $postat, $poinfo, $vars)
{
  $reposurl = $vars['reposurl'];
  $tl = $vars['total'];

  if ($poinfo[0] == '') {
    $flagimg=$vars['flagpath']."gb.png";
  } else {
    $flagimg=$vars['flagpath']."$poinfo[0].png";
  }

  echo "<td>$i</td>";
  echo "<td class=\"lang\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;<a href=\"$dlfile\">$poinfo[1]</a></td>";
  echo "<td class=\"lang\">&nbsp;</td>";
  echo "<td class=\"trans\">$tl</td>";
  echo "<td class=\"trans\">0.0%</td>";
  echo "<td class=\"graph\">&nbsp;</td>";
  echo "<td class=\"download\"><a href=\"$reposurl$postat[5]\">$postat[5]</a></td>";
  echo "<td class=\"download\">&nbsp;</td>";
}

function print_content_stat($i, $postat, $poinfo, $vars)
{
  $wc=150;
  $wp=$wc/100;

  $total=$vars['total'];
  $release=$vars['release'];
  $reposurl=$vars['reposurl'];
  $reposfile=$postat[5].'.po';
  $dlfile=$vars['downloadurl1']."LanguagePack_".$release."_".$poinfo[0].".exe".$vars['downloadurl2'];
  $fdate=date("Y-m-d",$postat[6]);

  $acc=$postat[4];
  $unt=$postat[3];
  $fuz=$postat[2];
  $tra=$postat[1];

  // Calculate width of bars
  $wa=round($wc*$acc/$total);
  $wf=round($wc*$fuz/$total);
  $wu=round($wc*$unt/$total);

  // Calculate width and percentage done. Do this before adjustments!
  $wt = $wc-$wa-$wf-$wu;
  $pt=number_format($wt/$wp, 1)."%";

  // Adjustments
  // make sure that each bar is at least 1px wide if it's value is > 0
  if (($wa<2) && ($acc>0)) $wa=2;
  if (($wf<2) && ($fuz>0)) $wf=2;
  if (($wu<2) && ($unt>0)) $wu=2;

  // Adjust total width accordingly
  $wt = $wc-$wa-$wf-$wu;

  // if completeness was rounded up to 100% and 
  // anything is missing, set completeness down to 99.9%
  if ( ($pt=="100.0%") && ($wa+$wf+$wu>0) )
    $pt="99.9%";

  if ($pt=="100.0%") {
    $title="Perfect :-)";
  } else {
    $title="tr:$tra&nbsp;fu:$fuz&nbsp;ut:$unt;&nbsp;$acc&nbsp;missing&nbsp;hotkeys";
  }
  $flagimg=$vars['flagpath']."$poinfo[0].png";

  // count fuzzies as translated, only for the display
  $tra=$tra+$fuz;

  echo "<td>$i</td>";
  echo "<td class=\"lang\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;<a href=\"$dlfile\">$poinfo[1]</a></td>";
  echo "<td class=\"lang\">$poinfo[0]</td>";
  echo "<td class=\"trans\">$tra</td>";
  echo "<td class=\"trans\">$pt</td>";
  echo "<td class=\"graph\">";
  echo "<img src=\"files/translated.png\" alt=\"tr\" title=\"$title\" width=\"$wt\" height=\"16\"/>";
  echo "<img src=\"files/missingaccelerator.png\" alt=\"mh\" title=\"$title\" width=\"$wa\" height=\"16\"/>";
  echo "<img src=\"files/fuzzy.png\" alt=\"fu\" title=\"$title\" width=\"$wf\" height=\"16\"/>";
  echo "<img src=\"files/untranslated.png\" alt=\"un\" title=\"$title\" width=\"$wu\" height=\"16\" />";
  echo "</td>";
  echo "<td class=\"download\"><a href=\"$reposurl$reposfile\">$reposfile</a></td>";
  echo "<td class=\"lang\">$fdate</td>";
}

function print_single_stat($i, $postat, $poinfo, $vars)
{
  if (($postat[0] > 0) || ($postat[1] == $postat[3])){
    echo "<tr class=\"error\">\n";
    print_blank_stat($i, $postat, $poinfo, $vars);
  }
  else if ($postat[1] == 0) {
    echo "<tr class=\"ok\">\n";
    print_blank_stat($i, $postat, $poinfo, $vars);
  }
  else {
    echo "<tr class=\"ok\">\n";
    print_content_stat($i, $postat, $poinfo, $vars);
  }
  echo "</tr>\n";
}

function print_all_stats($data, $country, $vars)
{
  $i=0;
  foreach ($data as $key => $postat)
  {
      $i++;
      print_single_stat($i, $postat, $country[$key], $vars);
  }
}

//------------------------------------
//
// The program starts here
//

print_header($vars);

// Print Alphabetical statistics
print_table_header('alpha', 'Languages ordered by ISO Code', $vars);
print_all_stats($data, $country, $vars);
print_table_footer();

// Convert Data into a list of columns
foreach ($data as $key => $row) {
   $errors[$key] = $row[0];
   $transl[$key] = $row[1];
   $fuzzy[$key] = $row[2];
   $untrans[$key] = $row[3];
   $accel[$key] = $row[4];
   $name[$key] = $row[5];
   $fdate[$key] = $row[6];
}

// Sort the data with volume descending, edition ascending
// Add $data as the last parameter, to sort by the common key
array_multisort($untrans, SORT_ASC, $transl, SORT_DESC, $fuzzy, SORT_ASC, $accel, SORT_ASC, $name, SORT_ASC, $data);


print_table_header('toplist', 'Languages by translation status', $vars);
print_all_stats($data, $country, $vars);
print_table_footer();


print_footer($vars);
?>
