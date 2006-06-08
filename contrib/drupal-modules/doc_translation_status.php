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
$vars['downloadurl1']=variable_get('tsvn_sf_prefix', '');
$vars['downloadurl2']=variable_get('tsvn_sf_append', '');
$vars['reposurl']="http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/Languages/";
$vars['flagpath']="/flags/world.small/";

$basename="Tortoise";
$template=$basename.".pot";

function print_d_header($vars)
{
?>

<div class="content">
<h2>Translations (in Revision <?php echo $vars['wcrev']; ?>)</h2>

<p>
This page is informing you about the documentation translation status of the current development version of TortoiseSVN, which is always ahead of the latest official release. The statistics are calculated for the HEAD revision and updated nightly. The last update was run at <b><?php echo $vars['update']; ?></b>.
</p>

<p>
The language pack installers can be downloaded from this page as well. The installers are always built for the last official release <b>(<?php echo $vars['release'] ?>)</b>. 
</p>

<p>
If you want to download the po file from the repository, either use <strong>guest/guest</strong> or your tigris.org user ID. 
</p>

<p>The manuals can be downloaded directly for the latest <a href="doc_release">release</a> or for the <a href="doc_nightly">developer builds</a> The developer builds are updated at irregular intervals.</p>
</p>

<?php
}

function print_d_footer($vars)
{
?>

<p>
<img src="siteicons/translated.png" alt="translated" title="translated" width="32" height="16"/> Translated <img src="siteicons/fuzzy.png" alt="fuzzy" title="fuzzy" width="32" height="16"/> Fuzzy <img src="siteicons/untranslated.png" alt="untranslated" title="untranslated" width="32" height="16" /> Untranslated
</p>
<p>
Translations were made by many people, you find them on the <a href="translator_credits">translator credits page</a>.
</p>

</div>

<?php
}

function print_d_table_header($name, $summary, $postat, $vars)
{
?>
<h2><?php echo $summary ?></h2>
<div class="table">
<table class="translations" summary="<?php echo $summary ?>">
<tr>
<th class="lang">Nr.</th>
<th class="lang">Language</th>
<th class="lang">ISO code</th>
<th colspan="2" class="trans">(<?php echo $postat[1] ?>) Complete</th>
<th class="graph">Graph</th>
<th class="download">Download .po file</th>
<th class="download">Last update</th>
</tr>
<?php
}

function print_d_table_footer()
{
?>
</table>
</div>
<div style="clear:both">&nbsp;<br/></div>
<?php
}

function print_d_blank_stat($i, $postat, $poinfo, $vars)
{
  $reposurl = $vars['reposurl'];
  $tl = $postat[1];

  if ($poinfo[0] == '') {
    $flagimg=$vars['flagpath']."gb.png";
  } else {
    $flagimg=$vars['flagpath']."$poinfo[2].png";
  }

  echo "<td>$i</td>";
  echo "<td class=\"lang\"><a href=\"$dlfile\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;$poinfo[3]</a></td>";
  echo "<td class=\"lang\">&nbsp;</td>";
  echo "<td class=\"trans\">$tl</td>";
  echo "<td class=\"trans\">0.0%</td>";
  echo "<td class=\"graph\">&nbsp;</td>";
  echo "<td class=\"download\"><a href=\"$reposurl$postat[6]\">$postat[6]</a></td>";
  echo "<td class=\"download\">&nbsp;</td>";
}

function print_d_content_stat($i, $postat, $poinfo, $vars)
{
  $wc=150;
  $wp=$wc/100;

  $release=$vars['release'];
  $reposurl=$vars['reposurl'];

  $total=$postat[1];
  $tra=$postat[2];
  $fuz=$postat[3];
  $unt=$postat[4];
  $reposfile=$postat[6].'.po';
  $fdate=date("Y-m-d",$postat[7]);
  $dlfile=$vars['downloadurl1']."LanguagePack_".$release."_".$poinfo[1].".exe".$vars['downloadurl2'];

  // Calculate width of bars
  $wf=round($wc*$fuz/$total);
  $wu=round($wc*$unt/$total);

  // Calculate width and percentage done. Do this before adjustments!
  $wt = $wc-$wf-$wu;
  $pt=number_format($wt/$wp, 1)."%";

  // Adjustments
  // make sure that each bar is at least 1px wide if it's value is > 0
  if (($wf<2) && ($fuz>0)) $wf=2;
  if (($wu<2) && ($unt>0)) $wu=2;

  // Adjust total width accordingly
  $wt = $wc-$wf-$wu;

  // if completeness was rounded up to 100% and 
  // anything is missing, set completeness down to 99.9%
  if ( ($pt=="100.0%") && ($wf+$wu>0) )
    $pt="99.9%";

  if ($pt=="100.0%") {
    $title="Perfect :-)";
  } else {
    $title="tr:$tra&nbsp;fu:$fuz&nbsp;ut:$unt;";
  }
  $flagimg=$vars['flagpath']."$poinfo[2].png";

  // count fuzzies as translated, only for the display
  $tra=$tra+$fuz;

  echo "<td>$i</td>";
  echo "<td class=\"lang\"><a href=\"$dlfile\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;$poinfo[3]</a></td>";
  echo "<td class=\"lang\">$poinfo[2]</td>";
  echo "<td class=\"trans\">$tra</td>";
  echo "<td class=\"trans\">$pt</td>";
  echo "<td class=\"graph\">";
  echo "<img src=\"siteicons/translated.png\" alt=\"tr\" title=\"$title\" width=\"$wt\" height=\"16\"/>";
  echo "<img src=\"siteicons/fuzzy.png\" alt=\"fu\" title=\"$title\" width=\"$wf\" height=\"16\"/>";
  echo "<img src=\"siteicons/untranslated.png\" alt=\"un\" title=\"$title\" width=\"$wu\" height=\"16\" />";
  echo "</td>";
  echo "<td class=\"download\"><a href=\"$reposurl$reposfile\">$reposfile</a></td>";
  echo "<td class=\"lang\">$fdate</td>";
}

function print_d_single_stat($i, $postat, $poinfo, $vars)
{
  if (($postat[0] > 0)){
    echo "<tr class=\"error\">\n";
    print_d_blank_stat($i, $postat, $poinfo, $vars);
  }
  else if ($postat[2] == 0) {
    echo "<tr class=\"ok\">\n";
    print_d_blank_stat($i, $postat, $poinfo, $vars);
  }
  else {
    echo "<tr class=\"ok\">\n";
    print_d_content_stat($i, $postat, $poinfo, $vars);
  }
  echo "</tr>\n";
}

function print_d_all_stats($data, $countries, $vars)
{
  $i=0;
  foreach ($data as $key => $postat)
  if ($postat[8] <> '')
  {
      $i++;
      print_d_single_stat($i, $postat, $countries[$key], $vars);
  }
}

//------------------------------------
//
// The program starts here
//

// Merge translation and country information into one array
$TortoiseSVN = array_merge_recursive($TortoiseSVN, $countries);

// Convert Data into a list of columns
foreach ($TortoiseSVN as $key => $row) {
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

// Add $TortoiseSVN as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $accel, $TortoiseSVN);

print_d_header($vars);

// Print Alphabetical statistics
print_d_table_header('TortoiseSVN', 'TortoiseSVN', $TortoiseSVN['zzz'], $vars);
print_d_all_stats($TortoiseSVN, $countries, $vars);
print_d_table_footer();

// Merge translation and country information into one array
$TortoiseMerge = array_merge_recursive($TortoiseMerge, $countries);

// Convert Data into a list of columns
foreach ($TortoiseMerge as $key => $row) {
   $merrors[$key] = $row[0];
   $mtotal[$key] = $row[1];
   $mtransl[$key] = $row[2];
   $mfuzzy[$key] = $row[3];
   $muntrans[$key] = $row[4];
   $maccel[$key] = $row[5];
   $mname[$key] = $row[6];
   $mfdate[$key] = $row[7];
   $mpotfile[$key] = $row[8];
   $mcountry[$key] = $row[10];
}

// Add $TortoiseMerge as the last parameter, to sort by the common key
array_multisort($mpotfile, $mcountry, $mtransl, $muntrans, $mfuzzy, $maccel, $TortoiseMerge);

print_d_table_header('TortoiseMerge', 'TortoiseMerge', $TortoiseMerge['zzz'], $vars);
print_d_all_stats($TortoiseMerge, $countries, $vars);
print_d_table_footer();


print_d_footer($vars);
?>
