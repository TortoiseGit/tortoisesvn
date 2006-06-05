<!--break-->
<?php
// index.php
//
// Main page.  Lists all the translations

//include("trans_data.inc");
//include("trans_countries.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_data.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_countries.inc");

$vars['release']=variable_get('tsvn_version', '');
$vars['build']=variable_get('tsvn_build', '');
$vars['downloadurl1']=variable_get('tsvn_sf_prefix', '');
$vars['downloadurl2']=variable_get('tsvn_sf_append', '');
$vars['reposurl']="http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/Languages/";
$vars['flagpath']="/flags/world.small/";
$basename="Tortoise";
$template=$basename.".pot";

function print_header($vars)
{
?>

<div class="content">
<h2>Translations (in Revision <?php echo $vars['wcrev']; ?>)</h2>

<p>
This page is informing you about the GUI translation status of the current development version of TortoiseSVN, which is always ahead of the latest official release. The statistics are calculated for the HEAD revision and updated nightly. The last update was run at <b><?php echo $vars['update']; ?></b>.
</p>

<p>
The language pack installers can be downloaded from this page as well. The installers are always built for the last official release <b>(<?php echo $vars['release'] ?>)</b>. 
</p>

<p>
If you want to download the po file from the repository, either use <strong>guest (no password)</strong> or your tigris.org user ID. 
</p>

<?php
}

function print_footer($vars)
{
?>
<p>
<img src="siteicons/translated.png" alt="translated" title="translated" width="32" height="16"/> Translated <img src="siteicons/missingaccelerator.png" alt="missing accelerator keys" title="missing accelerator keys" width="32" height="16"/> Missing accelerator keys <img src="siteicons/fuzzy.png" alt="fuzzy" title="fuzzy" width="32" height="16"/> Fuzzy <img src="siteicons/untranslated.png" alt="untranslated" title="untranslated" width="32" height="16" /> Untranslated
</p>
<p>
Translations were made by many people, you find them on the <a href="translator_credits">translator credits page</a>.
</p>

</div>

<?php
}

function print_table_header($name, $summary, $postat, $vars)
{
?>
<h2><?php echo $summary ?></h2>
<div class="table">
<table class="translations" summary="<?php echo $summary ?>">
<tr>
<th class="lang">Nr.</th>
<th class="lang">Download Installer (<?php echo $vars['release'] ?>)</th>
<th class="lang">ISO code</th>
<th colspan="2" class="trans">(<?php echo $postat[1] ?>) Complete</th>
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

function print_blank_stat($i, $postat, $vars)
{
  $tl = $postat[1];
  $reposurl = $vars['reposurl'].$postat[6];
  $fdate=date("Y-m-d",$postat[7]);
  $flagimg=$vars['flagpath']."$postat[9].png";

  echo "<td>$i</td>";
  echo "<td class=\"lang\"><a href=\"$dlfile\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;$postat[10]</a></td>";
  echo "<td class=\"lang\">&nbsp;</td>";
  echo "<td class=\"trans\">$tl</td>";
  echo "<td class=\"trans\">0.0%</td>";
  echo "<td class=\"graph\">&nbsp;</td>";
  echo "<td class=\"download\"><a href=\"$reposurl\">$postat[6]</a></td>";
  echo "<td class=\"lang\">$fdate</td>";
}

function print_content_stat($i, $postat, $vars)
{
  $wc=150;

  $release=$vars['release'];
  $build=$vars['build'];

  $total=$postat[1];
  $tra=$postat[2];
  $fuz=$postat[3];
//  $unt=abs($postat[1]-$postat[2]-$postat[3]);
  $unt=$postat[4];
  $acc=$postat[5];
  $reposfile=$postat[6].'.po';
  $reposurl=$vars['reposurl'].$reposfile;
  $fdate=date("Y-m-d",$postat[7]);

$dlfile=$vars['downloadurl1']."LanguagePack-".$release.".".$build."-win32-".$postat[9].".exe".$vars['downloadurl2'];
  $flagimg=$vars['flagpath']."$postat[9].png";


  // Calculate width of bars
  $wa=round($wc*$acc/$total);
  $wf=round($wc*$fuz/$total);
  $wu=round($wc*$unt/$total);

  // Calculate width. Do this before adjustments!
  $wt = $wc-$wa-$wf-$wu;

  // Calculate percentage done. 
  $pt=number_format(100*$wt/$wc, 1)."%";
  
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

  // count fuzzies as translated, only for the display
  $tra=$tra+$fuz;

  echo "<td>$i</td>";
  echo "<td class=\"lang\"><a href=\"$dlfile\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;$postat[10]</a></td>";
  echo "<td class=\"lang\">$postat[9]</td>";
  echo "<td class=\"trans\">$tra</td>";
  echo "<td class=\"trans\">$pt</td>";
  echo "<td class=\"graph\">";
  echo "<img src=\"siteicons/translated.png\" alt=\"tr\" title=\"$title\" width=\"$wt\" height=\"16\"/>";
  echo "<img src=\"siteicons/missingaccelerator.png\" alt=\"mh\" title=\"$title\" width=\"$wa\" height=\"16\"/>";
  echo "<img src=\"siteicons/fuzzy.png\" alt=\"fu\" title=\"$title\" width=\"$wf\" height=\"16\"/>";
  echo "<img src=\"siteicons/untranslated.png\" alt=\"un\" title=\"$title\" width=\"$wu\" height=\"16\" />";
  echo "</td>";
  echo "<td class=\"download\"><a href=\"$reposurl\">$reposfile</a></td>";
  echo "<td class=\"lang\">$fdate</td>";
}

function print_single_stat($i, $postat, $vars)
{
  if ($postat[0] > 0){
    echo "<tr class=\"error\">\n";
    print_blank_stat($i, $postat, $vars);
  }
  else if ($postat[8] == 1) {
    echo "<tr class=\"ok\">\n";
    print_blank_stat($i, $postat, $vars);
  }
  else {
    echo "<tr class=\"ok\">\n";
    print_content_stat($i, $postat, $vars);
  }
  echo "</tr>\n";
}

function print_all_stats($data, $vars)
{
  $i=0;
  foreach ($data as $key => $postat)
  {
      $i++;
      print_single_stat($i, $postat, $vars);
  }
}

//------------------------------------
//
// The program starts here
//


// Merge translation and country information into one array
$TortoiseGUI = array_merge_recursive($TortoiseGUI, $countries);

// Convert Data into a list of columns
foreach ($TortoiseGUI as $key => $row) {
   $errors[$key] = $row[0];
   $total[$key] = $row[1];
   $transl[$key] = $row[2];
   $fuzzy[$key] = $row[3];
   $untrans[$key] = $row[4];
   $accel[$key] = $row[5];
   $name[$key] = $row[6];
   $fdate[$key] = $row[7];
   $potfile[$key] = $row[8];
   $country[$key] = $row[10];
}

// Add $TortoiseGUI as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $accel, $TortoiseGUI);


print_header($vars);

// Print Alphabetical statistics
print_table_header('alpha', 'Languages ordered by country', $TortoiseGUI['zzz'], $vars);
print_all_stats($TortoiseGUI, $vars);
print_table_footer();

array_multisort($potfile, SORT_ASC, $transl, SORT_DESC, $untrans, SORT_ASC, $fuzzy, SORT_ASC, $accel, SORT_ASC, $country, SORT_ASC, $TortoiseGUI);

print_table_header('toplist', 'Languages by translation status', $TortoiseGUI['zzz'], $vars);
print_all_stats($TortoiseGUI, $vars);
print_table_footer();


print_footer($vars);
?>
