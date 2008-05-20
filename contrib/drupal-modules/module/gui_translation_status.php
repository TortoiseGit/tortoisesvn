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
$vars['reposurl']=variable_get('tsvn_repos_trunk', '').'Languages/';
$vars['flagpath']="/flags/world.small/";
$vars['showold']=TRUE;
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
If you want to download the po file from the repository, either use <strong>guest (no password)</strong> or your tigris.org user ID. 
</p>

<?php
}

//------------------------------------
//
// The program starts here
//


// Merge translation and country information into one array
$TortoiseGUI = array_merge_recursive($countries, $TortoiseGUI);

// Convert Data into a list of columns
foreach ($TortoiseGUI as $key => $row) {
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

// Add $TortoiseGUI as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $accel, $TortoiseGUI);

print_header($vars);

// Print Alphabetical statistics
print_table_header('sort_alpha', 'Languages ordered by country', $TortoiseGUI['zzz'], $vars);
print_all_stats($TortoiseGUI, $vars);
print_table_footer();

array_multisort($potfile, SORT_ASC, $transl, SORT_DESC, $untrans, SORT_ASC, $fuzzy, SORT_ASC, $accel, SORT_ASC, $country, SORT_ASC, $TortoiseGUI);

print_table_header('sort_status', 'Languages by translation status', $TortoiseGUI['zzz'], $vars);
print_all_stats($TortoiseGUI, $vars);
print_table_footer();

print_footer($vars);

?>
