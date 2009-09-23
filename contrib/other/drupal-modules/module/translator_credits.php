<!--break-->
<?php
//
// Drupal translator credits page
// loaded into "http://tortoisesvn.net/translator_credits"
//
// Copyright (C) 2004-2009 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// $Author$
// $Date$
// $Rev$
//
// Author: Lübbe Onken 2004-2009
//

include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/trans_data_trunk.inc");
include("/var/www/vhosts/default/htdocs/modules/tortoisesvn/tortoisevars.inc");

function t_print_header($tsvn_var)
{
?>

<div class="content">
<p>
On this page we want to give credit to everyone who has contributed to the many translations we now have. I hope I haven't forgotten a translator... Thanks everybody!
</p>

<?php
}

function t_print_footer($tsvn_var)
{
?>

</div>

<?php
}

function t_print_table_header($summary, $tsvn_var)
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

function t_print_table_footer()
{
?>
</table>
</div>
<div style="clear:both">&nbsp;<br/></div>
<?php
}

function t_print_content_stat($i, $info, $tsvn_var)
{
  $release=$tsvn_var['release'];
  $build=$tsvn_var['build'];
  $dlfile=$tsvn_var['url1']."LanguagePack_".$release.".".$build."-win32-".$info[2].".msi".$tsvn_var['url2'];

  if ($info[0] == 0) 
    $flagimg=$tsvn_var['flagpath']."gb.png";
  else
    $flagimg=$tsvn_var['flagpath'].$info[2].".png";

  echo "<td>$i</td>";
  echo "<td class=\"lang\"><img src=\"$flagimg\" height=\"12\" width=\"18\" />&nbsp;<a href=\"$dlfile\">$info[3]</a></td>";
  echo "<td class=\"lang\">$info[4]</td>";
}

function t_print_all_stats($countries, $tsvn_var)
{
  $i=0;
  foreach ($countries as $key => $langinfo)
    {
      $i++;
      echo "<tr>";
      t_print_content_stat($i, $langinfo, $tsvn_var);
      echo "</tr>";
    }
}

//------------------------------------
//
// The program starts here
//

t_print_header($tsvn_var);

// Print Alphabetical statistics
t_print_table_header('Translator credits', $tsvn_var);
t_print_all_stats($countries, $tsvn_var);
t_print_table_footer();

t_print_footer($tsvn_var);
?>
