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

function print_header($vars)
{
?>

<div class="content">
<p>
On this page we want to give credit to everyone who has contributed to the many translations we now have. I hope I haven't forgotten a translator... Thanks everybody!
</p>

<?php
}

function print_footer($vars)
{
?>

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
<th class="lang">Language</th>
<th class="lang">Translator(s)</th>
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

function print_content_stat($i, $postat, $poinfo, $vars)
{
  $dlfile=$vars['downloadurl1']."LanguagePack_".$vars['release']."_".$poinfo[1].".exe".$vars['downloadurl2'];

  if ($poinfo[0] != '') {
    $flagimg=$vars['flagpath']."$poinfo[2].png";

    echo "<td>$i</td>";
    echo "<td class=\"lang\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;<a href=\"$dlfile\">$poinfo[3]</a></td>";
    echo "<td class=\"lang\">$poinfo[4]</td>";
  }
}

function print_all_stats($data, $countries, $vars)
{
  $i=0;
  foreach ($data as $key => $postat)
  {
      $i++;
      echo "<tr>";
      print_content_stat($i, $postat, $countries[$key], $vars);
      echo "</tr>";
  }
}

//------------------------------------
//
// The program starts here
//

print_header($vars);

// Print Alphabetical statistics
print_table_header('alpha', 'Translator credits', $vars);
print_all_stats($TortoiseGUI, $countries, $vars);
print_table_footer();

print_footer($vars);
?>
