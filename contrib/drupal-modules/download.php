<?php

include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_data.inc");
include("/home/groups/t/to/tortoisesvn/htdocs/includes/trans_countries.inc");

$v['release']=variable_get('tsvn_version', '');
$v['build']=variable_get('tsvn_build', '');
$v['build_x64']=variable_get('tsvn_build_x64', '');
$v['svnver']=variable_get('tsvn_svnlib', '');
$v['sf_release_id']=variable_get('tsvn_sf_release_binary', '');
$v['sf_project']=variable_get('tsvn_sf_project', '');
$v['url1']=variable_get('tsvn_sf_prefix', '');
$v['url2']=variable_get('tsvn_sf_append', '');
$v['flagpath']="/flags/world.small/";

$w['w32']=$v['release'].".".$v['build']."-win32"; 
$w['w32wrong']=$v['release'].".".$v['build'].""; 
$w['x64']=$v['release'].".".$v['build_x64']."-x64"; 

function get_changelog($v)
{
$t_ln="http://sourceforge.net/project/shownotes.php?release_id=".$v['sf_release_id'];
return "<a href=\"$t_ln\">changelog</a>";
}

function get_installer($v, $w)
{
$t_ln="TortoiseSVN-".$w."-svn-".$v['svnver'].".msi";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$t_ln</a>" ;
}

function get_checksum($v, $w)
{
$t_ln="TortoiseSVN-".$w."-svn-".$v['svnver'].".md5";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$t_ln</a>";
}

function get_langpack($l, $n, $v, $w)
{
$t_ln="LanguagePack-".$w."-".$l.".exe";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$n</a>";
}

function print_langpack($i, $postat, $v, $w)
{
  
  $flagimg=$v['flagpath']."$postat[10].png";
  $dlfile32=get_langpack($postat[10], $postat[11], $v, $w['w32']);
  $dlfile64=get_langpack($postat[10], $postat[11], $v, $w['x64']);
  if ($postat[9]=="1")   {
   $t_ln="SpellChecker_".$postat[10].".exe";
   $dlfilechecker="<a href=\"".$v['url1'].$t_ln.$v['url2']."\">Spellchecker</a>";
	} else {
   $dlfilechecker="";
  }
  
  echo "<tr>";
  echo "<td>$i</td>";
  echo "<td><img src=\"$flagimg\" height=\"12\" width=\"18\" /></td>";
  echo "<td>$dlfile32</td>";
  echo "<td>$dlfile64</td>";
  echo "<td>$dlfilechecker</td>";
  echo "</tr>";
}

//------------------------------------
//
// The program starts here
//

?>
<h1>The current version is <?php echo $v['release'] ?>.</h1>
<p>
For detailed info on what's new, read the <?php echo get_changelog($v); ?>.
</p>
<p>
This page points to installers for 32 bit and 64 bit operating systems. Please make sure that you choose the right installer for your PC. Otherwise the setup will fail.
</p>
<p>
<div class="table">
<table>
<tr>
<td>&nbsp;</td>
<th colspan="2">Download Application</th>
</tr>
<tr>
<th>32 Bit</th>
<td><?php echo get_installer($v,$w['w32wrong']) ?></td>
<td>Installer</td>
</tr>
<tr>
<td>&nbsp;</td>
<td><?php echo get_checksum($v,$w['w32wrong']) ?></td>
<td>MD5 checksum</td>
</tr>
<tr>
<th>64 Bit</th>
<td><?php echo get_installer($v,$w['x64']) ?></td>
<td>Installer</td>
</tr>
<tr>
<td>&nbsp;</td>
<td><?php echo get_checksum($v,$w['x64']) ?></td>
<td>MD5 checksum</td>
</tr>
</table>
</p>
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
<h2>Language packs</h2>
<div class="table">
<table>
<tr>
<th class="lang">&nbsp;</th>
<th class="lang">&nbsp;</th>
<th class="lang">32 Bit</th>
<th class="lang">64 Bit</th>
<th class="lang">Spellchecker</th>
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

<h1>Older Releases</h1>
<ul>
<li>Older releases are available from the <a href="http://sourceforge.net/project/showfiles.php?group_id=<?php print $v['sf_project']; ?>">Sourceforge files</a> section.</li>
</ul>

<h1>Nightly Builds</h1>
<ul>
<li><a href="http://mapcar.org/tsvn-snapshots/latest/">Nightly builds</a> are available too. These are for testing only. Please read the <a href="http://mapcar.org/tsvn-snapshots/latest/readme.txt">Readme.txt</a> first.</li>

<!--break-->
