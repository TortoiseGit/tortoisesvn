package someproject.model.decoder;

public class TT2Decoder {
    
    int getTraceYear ()
    {
    	return trace_year; // year not available from trace header (only month+day+time), so assume current year. 
    }
    
}
