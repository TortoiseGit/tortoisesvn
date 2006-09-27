<!--break-->
<?php
// index.php
//
// Main page.  Lists all the translations

//include("trans_data.inc");
//include("trans_countries.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_data.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_countries.inc");

$vars['wc']=100;
$vars['release']=variable_get('tsvn_version', '');
$vars['build']=variable_get('tsvn_build', '');
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
If you want to download the po file from the repository, either use <strong>guest (no password)</strong> or your tigris.org user ID. 
</p>

<p>The manuals can be downloaded directly for the latest <a href="doc_release">release</a> or for the <a href="doc_nightly">developer builds</a> The developer builds are updated at irregular intervals.</p>
</p>

<?php
}

function print_d_all_stats($data, $vars)
{
  $i=0;
  foreach ($data as $key => $postat)
  // Check if the translation exists (empty column in array_merge_recursive)
  if ($postat[8] <> '')
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
   $country[$key] = $row[11];
}

// Add $TortoiseSVN as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $accel, $TortoiseSVN);

print_d_header($vars);

// Print Alphabetical statistics
print_table_header('TortoiseSVN', 'TortoiseSVN', $TortoiseSVN['zzz'], $vars);
print_d_all_stats($TortoiseSVN, $vars);
print_table_footer();

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
   $mcountry[$key] = $row[11];
}

// Add $TortoiseMerge as the last parameter, to sort by the common key
array_multisort($mpotfile, $mcountry, $mtransl, $muntrans, $mfuzzy, $maccel, $TortoiseMerge);

print_table_header('TortoiseMerge', 'TortoiseMerge', $TortoiseMerge['zzz'], $vars);
print_d_all_stats($TortoiseMerge, $vars);
print_table_footer();


print_footer($vars);
?>
