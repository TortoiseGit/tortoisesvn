<?php

include("/var/www/vhosts/default/htdocs/includes/trans_data.inc");
include("/var/www/vhosts/default/htdocs/includes/trans_countries.inc");

$v['release']=variable_get('tsvn_version', '');
$v['sf_project']=variable_get('tsvn_sf_project', '');
$v['url1']=variable_get('tsvn_sf_prefix', '');
$v['url2']=variable_get('tsvn_sf_append', '');
$v['flagpath']="/flags/world.small/";
$v['devurl']='http://www.tortoisesvn.net/docs/nightly/';


function print_langpack($i, $postat, $v, $w)
{
  
  $flagimg=$v['flagpath']."$postat[10].png";
  
  if ( ($postat[9] & "10") <> "0")   {
   $t_ts="TortoiseSVN-".$v['release'].'-'.$postat[10].".pdf";
   $t_tm="TortoiseMerge-".$v['release'].'-'.$postat[10].".pdf";

   $relTSVN="<a href=\"".$v['url1'].$t_ts.$v['url2']."\">TSVN</a>";
   $relTMerge="<a href=\"".$v['url1'].$t_tm.$v['url2']."\">TMerge</a>";
   $devTSVN="<a href=\"".$v['devurl'].$t_ts."\">TSVN</a>";
   $devTMerge="<a href=\"".$v['devurl'].$t_tm."\">TMerge</a>";

   echo "<tr>";
   echo "<td><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;$postat[11]</td>";
   echo "<td>$relTSVN</td>";
   echo "<td>$relTMerge</td>";
   echo "<td>$devTSVN</td>";
   echo "<td>$devTMerge</td>";
   echo "</tr>";
  }

}

//------------------------------------
//
// The program starts here
//

?>
<h1>The latest official release is <?php echo $v['release'] ?>.</h1>

On this page you can download separate PDF versions of the TortoiseSVN documentation.

<dl>
<dt>Release version</dt>
<dd>If you have TortoiseSVN installed, you can simply press the F1 key in any dialog to start up the help. That help is the same as the documentation you find here.</dd>
<dt>Developer version</dt>
<dd>Please note that these docs aren't updated on a daily basis but very irregularily.</dd>
</dl>

<?php

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
   $spellcheck[$key] = $row[9];
   $country[$key] = $row[11];
}

// Add $TortoiseGUI as the last parameter, to sort by the common key
array_multisort($potfile, $country, $transl, $untrans, $fuzzy, $accel, $TortoiseGUI);

?>
<p>
<h2>Manuals</h2>
<div class="table">
<table>
<tr>
<th class="lang">Country</th>
<th class="lang" colspan="2">Release version (PDF)</th>
<th class="lang" colspan="2">Developer version (PDF)</th>
</tr>

<?php
$i=0;
foreach ($TortoiseGUI as $key => $postat)
{
  $i++;
  if ($postat[8] == "0")
    print_langpack($i, $postat, $v, $w);
}
?>

</table>
</p>

</div>

<h1>Older Manuals</h1>
<ul>
<li>Older releases are available from the <a href="http://sourceforge.net/project/showfiles.php?group_id=<?php print $v['sf_project']; ?>">Sourceforge files</a> section.</li>
</ul>

<!--break-->
