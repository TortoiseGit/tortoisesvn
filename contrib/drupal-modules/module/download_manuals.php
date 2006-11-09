<?php

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_trunk.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_countries.inc");

$v['release']=variable_get('tsvn_version', '');
$v['sf_project']=variable_get('tsvn_sf_project', '');
$v['url1']=variable_get('tsvn_sf_prefix', '');
$v['url2']=variable_get('tsvn_sf_append', '');
$v['flagpath']="/flags/world.small/";
$v['devurl']='http://www.tortoisesvn.net/docs/nightly/';
$v['relurl']='http://www.tortoisesvn.net/docs/release/';


function print_langpack($i, $postat, $v, $w, $b_release)
{
  $flagimg=$v['flagpath']."$postat[10].png";

  if ( (($postat[9] & "10") <> "0") || ($postat[10] == "gb") ) {

    if ($postat[10] == "gb") {
      $m_cc = "en";
      $m_cn = "English";
    } else {
      $m_cc = $postat[10];
      $m_cn = $postat[11];
    }

    $ts_pdf="TortoiseSVN-".$v['release'].'-'.$m_cc.".pdf";
    $tm_pdf="TortoiseMerge-".$v['release'].'-'.$m_cc.".pdf";
    $ts_htm="TortoiseSVN_".$m_cc."/index.html";
    $tm_htm="TortoiseMerge_".$m_cc."/index.html";

    if ($b_release==TRUE) {
      $pdfTSVN="<a href=\"".$v['url1'].$ts_pdf.$v['url2']."\">PDF</a>";
      $htmTSVN="<a href=\"".$v['relurl'].$ts_htm."\">HTML</a>";
      $pdfTMerge="<a href=\"".$v['url1'].$tm_pdf.$v['url2']."\">PDF</a>";
      $htmTMerge="<a href=\"".$v['relurl'].$tm_htm."\">HTML</a>";
    } else {
      $pdfTSVN="<a href=\"".$v['devurl'].$ts_pdf."\">PDF</a>";
      $htmTSVN="<a href=\"".$v['devurl'].$ts_htm."\">HTML</a>";
      $pdfTMerge="<a href=\"".$v['devurl'].$tm_pdf."\">PDF</a>";
      $htmTMerge="<a href=\"".$v['devurl'].$tm_htm."\">HTML</a>";
    }

    echo "<tr>";
    echo "<td><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;$m_cn</td>";
    echo "<td>$pdfTSVN</td>";
    echo "<td>$htmTSVN</td>";
    echo "<td>$pdfTMerge</td>";
    echo "<td>$htmTMerge</td>";
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
The manuals are also made available in multi page HTML format if you prefer to browse them online. 

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
<h1>Manuals (release version)</h1>
If you have TortoiseSVN installed, you can simply press the F1 key in any dialog to start up the help. That help is the same as the documentation you find here.
<div class="table">
<table>
<tr>
<th class="lang">Country</th>
<th class="lang" colspan="2">TortoiseSVN</th>
<th class="lang" colspan="2">TortoiseMerge</th>
</tr>
<?php
$i=0;
foreach ($TortoiseGUI as $key => $postat)
{
  $i++;
  print_langpack($i, $postat, $v, $w, TRUE);
}
?>
</table>
</div>

<h1>Manuals (developer version)</h1>
Please note that these docs aren't updated nightly but very irregularly.
<div class="table">
<table>
<tr>
<th class="lang">Country</th>
<th class="lang" colspan="2">TortoiseSVN</th>
<th class="lang" colspan="2">TortoiseMerge</th>
</tr>
<?php
$i=0;
foreach ($TortoiseGUI as $key => $postat)
{
  $i++;
  print_langpack($i, $postat, $v, $w, FALSE);
}
?>
</table>
</div>

<h1>Older Manuals</h1>
<ul>
<li>Older releases are available from the <a href="http://sourceforge.net/project/showfiles.php?group_id=<?php print $v['sf_project']; ?>">Sourceforge files</a> section.</li>
</ul>

<!--break-->
