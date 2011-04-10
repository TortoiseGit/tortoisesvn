<?php
require_once "TablePrint.php";

class Table
{
   // members
   public $header;
   public $data;

   // init table data
   public function Table()
   {
      $this->header = array();
      $this->data = array();
   }

   function RemoveRow($column)
   {
   }

   function RemoveColumn($column)
   {
   }

   function importMysqlResult($result)
   {
      $this->Table();
      if($result === false)
      {
         return;
      }
      for($i = 0; $i < mysql_num_fields($result); $i++)
      {
         $this->header[$i] = mysql_field_name($result, $i);
      }

      for($i = 0; $i < mysql_num_rows($result); $i++)
      {
         $this->data[$i] = mysql_fetch_row($result);
      }
   }

   function import_from_table_by_headers($table, $columns)
   {
   }

   function alterCell($row, $column, $newdata)
   {
      if(is_string($column))
      {
         $column = array_search($column, $this->header);
      }
      $this->data[$row][$column] = $newdata;
   }

   function getCell($row, $column)
   {
      if(is_string($column))
      {
         $column = array_search($column, $this->header);
      }
      return $this->data[$row][$column];
   }

   function Output()
   {
      PrintTable($this->header, $this->data);
   }
}
?>