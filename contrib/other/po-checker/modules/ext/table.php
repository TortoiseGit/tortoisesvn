<?php
require_once "tableprint.php";

class Table{
	// members
	public $header;
	public $data;

	// init table data
	public function Table(){
		$this->header=array();
		$this->data=array();
	}

	function RemoveRow($column){
	}

	function RemoveColumn($column){
	}

	function importMysqlResult($result){
		$this->Table();
		for ($i=0; $i<mysql_num_fields($result); $i++) {
			$this->header[$i]=mysql_field_name($result, $i);
		}

		for ($i=0; $i<mysql_num_rows($result); $i++) {
			$this->data[$i]=mysql_fetch_row($result);
		}
	}

	function import_from_table_by_headers($table, $columns) {
	}

	function alterCell($row, $column, $newdata) {
	}

	function Output() {
		PrintTable($this->header, $this->data);
	}
}
//php?>
