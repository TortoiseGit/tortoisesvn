<?php
//
// Drupal download page for TortoiseSVN
//
// Copyright (C) 2004-2009 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// $Author$
// $Date$
// $Rev$
//
// Author: Lübbe Onken 2004-2009
//         Stefan Küng 2004-2008
//

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_branch.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/tortoisevars.inc");

$w['w32']=$tsvn_var['release'].".".$tsvn_var['build']."-win32"; 
$w['w32wrong']=$tsvn_var['release'].".".$tsvn_var['build'].""; 
$w['x64']=$tsvn_var['release'].".".$tsvn_var['build']."-x64"; 

if (!function_exists('get_installer')) {
function get_installer($v, $w)
{
$t_ln="TortoiseSVN-".$w."-svn-".$v['svnver'].".msi";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$t_ln</a>" ;
}
}

if (!function_exists('get_checksum')) {
function get_checksum($v, $w)
{
$t_ln="TortoiseSVN-".$w."-svn-".$v['svnver'].".msi.asc";
return "<a href=\"http://tortoisesvn.net/files/".$t_ln."\">$t_ln</a>";
}
}

if (!function_exists('get_langpack')) {
function get_langpack($l, $n, $v, $w)
{
$t_ln="LanguagePack_".$w."-".$l.".msi";
return "<a href=\"".$v['url1'].$t_ln.$v['url2']."\">$n</a>";
}
}

if (!function_exists('print_langpack')) {
function print_langpack($i, $postat, $v, $w)
{
  $infobits=$postat[1];
  $lang_cc=$postat[2];
  $lang_name=$postat[3];
  $flagimg=$v['flagpath'].$lang_cc.".png";
  $flagtag="<img src=\"$flagimg\" height=\"12\" width=\"18\" alt=\"$lang_name flag\"/>";
  
  $dlfile32=get_langpack($lang_cc, 'Setup', $v, $w['w32']);
  $dlfile64=get_langpack($lang_cc, 'Setup', $v, $w['x64']);
  
  if ( ($infobits & "010") <> "0") {
   $t_ts="TortoiseSVN-".$v['release'].'-'.$lang_cc.".pdf";
   $dlmanTSVN="<a href=\"".$v['url1'].$t_ts.$v['url2']."\">TSVN</a>";
  } else {
   $dlmanTSVN="";
  }

  if ( ($infobits & "100") <> "0") {
   $t_tm="TortoiseMerge-".$v['release'].'-'.$lang_cc.".pdf";
   $dlmanTMerge="<a href=\"".$v['url1'].$t_tm.$v['url2']."\">TMerge</a>";
  } else {
   $dlmanTMerge="";
  }

  echo "<tr class=\"stat_ok\">";
  echo "<td>$i</td>";
  //echo "<td>$flagtag&nbsp;$lang_name</td>"; 
  echo "<td>$lang_name</td>";
  echo "<td>$dlfile32</td>";
  echo "<td>$dlfile64</td>";
  echo "<td>$dlmanTSVN</td>";
  echo "<td>$dlmanTMerge</td>";
  echo "</tr>";
}
}

//------------------------------------
//
// The program starts here
//

?>
<h1>The current version is <?php echo $tsvn_var['release'] ?>.</h1>
<p>
For detailed info on what's new, read the <a href="http://tortoisesvn.tigris.org/ChangeLog.txt">changelog</a> and the <a href="http://tortoisesvn.tigris.org/tsvn_1.6_releasenotes.html">release notes</a>.
</p>
<p>
This page points to installers for 32 bit and 64 bit operating systems. Please make sure that you choose the right installer for your PC. Otherwise the setup will fail.
</p>
<p>
We got reports that upgrading sometimes does not work properly. If you have problems after updating TortoiseSVN, just uninstall it, reboot, and then install it again.
</p>
<p>
Note for x64 users: you can install both the 32 and 64-bit version side by side. This will enable the TortoiseSVN features also for 32-bit applications.
</p>

<div class="table">
<table>
<tr>
<td>&nbsp;</td>
<th colspan="2">Download Application</th>
</tr>
<tr>
<th>32 Bit</th>
<td><?php echo get_installer($tsvn_var,$w['w32']) ?></td>
<td>Installer</td>
</tr>
<tr>
<th>64 Bit</th>
<td><?php echo get_installer($tsvn_var,$w['x64']) ?></td>
<td>Installer</td>
</tr>
</table>
</div>

<p>


To verify the file integrity follow <a href="http://tortoisesvn.net/msiverify">these instructions</a>
</p>

<br />
<script type="text/javascript"><!--
google_ad_client = "pub-0430507460695576";
/* 300x250, tsvn.net inPage */
google_ad_slot = "5167477883";
google_ad_width = 300;
google_ad_height = 250;
//-->
</script>
<script type="text/javascript"
src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>
<?php

// Convert Data into a list of columns
foreach ($countries as $key => $row) {
   $potfile[$key] = $row[0];
   $country[$key] = $row[3];
}

// Add $countries as the last parameter, to sort by the common key
array_multisort($potfile, $country, $countries);

?>

<h2>Language packs</h2>
<div class="table">
<table class="translations">
<tr>
<th class="lang">&nbsp;</th>
<th class="lang">Country</th>
<th class="lang">32 Bit</th>
<th class="lang">64 Bit</th>
<th class="lang" colspan="2">Separate manual (PDF)</th>
</tr>

<?php
  $i=0;
  foreach ($countries as $key => $postat)
    if ($postat[0] == "1" ) {
      $i++;
      print_langpack($i, $postat, $tsvn_var, $w);
    }
?>

</table>
</div>

<h2>Tools</h2>
<p>We also provide some tools you already get together with TortoiseSVN as standalone applications for those which don't use TortoiseSVN as their Subversion client.</p>
<p>Our diff/merge tools TortoiseMerge (for text file diffs and merges), TortoiseIDiff (for diffing image files) are zipped into one package, and SubWCRev is also available.</p>
<p>You can download these tools from the <a href="https://sourceforge.net/project/showfiles.php?group_id=138498&package_id=281312">SourceForge download page</a>
<p>Note: <em>DO NOT</em> install these tools if you already have TortoiseSVN installed - you already <em>have</em> them.</p>



<h1>Forthcoming Releases</h1>
<p>To find out what is happening with the project and when you can expect the next release, take a look at our <a href="/status">project status</a> page.</p>

<h1>Release Candidates</h1>
<p>We maintain ongoing <a href="http://nightlybuilds.tortoisesvn.net/1.6.x/">Release Candidates</a>
as well. These contain the latest official release plus latest bugfixes. They are not built nightly, but on demand from the current release branch. If you find that a certain bug has been fixed and you do not want to wait until the next release, install one of these. You would also help us tremendously by installing and testing release candidates.
Please read <a href="http://nightlybuilds.tortoisesvn.net/1.6.x/Readme.txt">Readme.txt</a> first.</p>

<h1>Nightly Builds</h1>
<p><a href="http://nightlybuilds.tortoisesvn.net/latest/">Nightly Builds</a> are available too. They are built from the current development head and are for testing only. Please read <a href="http://nightlybuilds.tortoisesvn.net/latest/Readme.txt">Readme.txt</a> first.</p>

<h1>Older Releases</h1>
<p>Older releases are available from the <a href="http://sourceforge.net/project/showfiles.php?group_id=<?php print $tsvn_var['sf_project']; ?>">Sourceforge files</a> section.</p>

<h1>Sourcecode</h1>
<p>TortoiseSVN is under the GPL license. That means you can get the whole sourcecode and build the program yourself.
<br />
The sourcecode is hosted on <a href="http:/code.google.com/hosting">Google Code</a> in our own Subversion repository. You can browse the sourcecode with your favorite webbrowser directly on the <a href="http://code.google.com/p/tortoisesvn/source/browse/">repository</a>. 
<br />
If you have TortoiseSVN installed, you can check out the whole sourcecode by clicking on the tortoise icon below:
<br />
<a href="tsvn:http://tortoisesvn.googlecode.com/svn/trunk/"><img src="/files/TortoiseCheckout.png" alt="Tortoise Icon"/></a></p>

<script type="text/javascript"><!--
google_ad_client = "pub-0430507460695576";
/* 300x250, tsvn.net inPage */
google_ad_slot = "5167477883";
google_ad_width = 300;

google_ad_height = 250;
//-->
</script>


<script type="text/javascript"
src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>
<!--break-->
