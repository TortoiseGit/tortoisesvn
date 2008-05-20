<!--break-->
<?php
// index.php
//
// Main page.  Lists all the translations

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_trunk.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_countries.inc");

$vars['release']=variable_get('tsvn_version', '');
$vars['build']=variable_get('tsvn_build', '');
$vars['downloadurl1']=variable_get('tsvn_sf_prefix', '');
$vars['downloadurl2']=variable_get('tsvn_sf_append', '');
$vars['reposurl']=variable_get('tsvn_repos_trunk', '').'doc/po/';
$vars['flagpath']="/flags/world.small/";
$vars['showold']=TRUE;

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
If you want to download the po file from the repository, either use <strong>guest (no password)</strong> or your tigris.org user ID. 
</p>

<?php
}

//------------------------------------
//
// The program starts here
//

// Merge translation and country information into one array
$TortoiseSVN = array_merge_recursive($countries, $TortoiseSVN);

// Convert Data into a list of columns
foreach ($TortoiseSVN as $key => $row) {
   $potfile[$key] = $row[0];
   $country[$key] = $row[3];
   $errors[$key] = $row[5];
   $total[$key] = $row[6];
   $transl[$key] = $row[7];
   $fuzzy[$key] = $row[8];
   $untrans[$key] = $row[9];
   $accel[$key] = $row[10];
   $name[$key] = $row[11];
   $fdate[$key] = $row[12];
}

// Add $TortoiseSVN as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $TortoiseSVN);

print_d_header($vars);

// Print Alphabetical statistics
print_table_header('TortoiseSVN', 'TortoiseSVN', $TortoiseSVN['zzz'], $vars);
print_all_stats($TortoiseSVN, $vars);
print_table_footer();

// Merge translation and country information into one array
$TortoiseMerge = array_merge_recursive($countries, $TortoiseMerge);

// Convert Data into a list of columns
foreach ($TortoiseMerge as $key => $row) {
   $mpotfile[$key] = $row[0];
   $mcountry[$key] = $row[3];
   $merrors[$key] = $row[5];
   $mtotal[$key] = $row[6];
   $mtransl[$key] = $row[7];
   $mfuzzy[$key] = $row[8];
   $muntrans[$key] = $row[9];
   $maccel[$key] = $row[10];
   $mname[$key] = $row[11];
   $mfdate[$key] = $row[12];
}

// Add $TortoiseMerge as the last parameter, to sort by the common key
array_multisort($mpotfile, $mcountry, $mtransl, $muntrans, $mfuzzy, $maccel, $TortoiseMerge);

print_table_header('TortoiseMerge', 'TortoiseMerge', $TortoiseMerge['zzz'], $vars);
print_all_stats($TortoiseMerge, $vars);
print_table_footer();

print_footer($vars);

?>
