<!--break-->
<?php
// index.php
//
// Main page.  Lists all the translations

include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_data.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_countries.inc");

$vars['release']=variable_get('tsvn_version', '');
$vars['downloadurl1']=variable_get('tsvn_sf_prefix', '');
$vars['downloadurl2']=variable_get('tsvn_sf_append', '');
$vars['reposurl']="http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/Languages/";
$vars['flagpath']="/flags/world.small/";

$basename="Tortoise";
$template=$basename.".pot";

function print_content_stat($i, $postat, $poinfo, $vars)
{

  $dlfile=$vars['downloadurl1']."LanguagePack_".$release."_".$poinfo[0].".exe".$vars['downloadurl2'];

if ($poinfo[0] == '') {
    $flagimg=$vars['flagpath']."gb.png";
  } else {
    $flagimg=$vars['flagpath']."$poinfo[2].png";
  }

  echo "<a href=\"$dlfile\"><img src=\"$flagimg\" height=\"12\" width=\"18\" alt=\"$poinfo[1]\" title=\"$poinfo[3]\" /></a>&nbsp;";
}

function print_single_stat($i, $postat, $poinfo, $vars)
{
  if (($postat[0] > 0) || ($postat[1] == $postat[3])){
    // error
    print_content_stat($i, $postat, $poinfo, $vars);
  }
  else if ($postat[1] == 0) {
    // no translations
//    print_content_stat($i, $postat, $poinfo, $vars);
  }
  else {
    // everything ok
    print_content_stat($i, $postat, $poinfo, $vars);
  }
}

function print_all_stats($data, $countries, $vars)
{
  $i=0;
  foreach ($data as $key => $postat)
  {
      $i++;
      print_single_stat($i, $postat, $countries[$key], $vars);
  }
}

//------------------------------------
//
// The program starts here
//
?>

<div class="content">
<h2>TortoiseSVN <?php echo $vars['release'] ?> Translations</h2>

<p>
The following <?php echo count($TortoiseGUI); ?> languages are supported by TortoiseSVN
</p>
<p>
<?php print_all_stats($TortoiseGUI, $countries, $vars) ?>
</p>
<p>
Find out more on our <a href="?q=translation_status">Translation status page</a>.
See how many <a href="?q=translator_credits">volunteers</a> have contributed a translation.
</p>
</div>
