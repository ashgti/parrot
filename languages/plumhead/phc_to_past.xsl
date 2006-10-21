<?xml version='1.0'?>
<xsl:stylesheet  xmlns:phc="http://www.phpcompiler.org/phc-1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!--

$Id$

Take PHP Abstract syntax tree as generated by phc and generate PIR
that sets up PAST.

-->

<xsl:output method='text' />

<xsl:template match="phc:AST_php_script">
  <xsl:apply-templates/>

.sub _slurp_file                                                  
  .param string filename                                          
                                                                  
  .sym pmc filehandle                                             
  .sym string content                                             
  filehandle = open filename, '&lt;'                                 
  unless filehandle goto ERR_NO_FILE                              
  content = read filehandle, 65535                                
  close filehandle                                                
                                                                  
  .return ( content )                                             
                                                                  
  ERR_NO_FILE:                                                    
    print 'Unable to open file: '                                 
    say filename                                                  
    end                                                           
                                                                  
.end                                                              
                                                                  
</xsl:template>

<xsl:template match="phc:attrs">
</xsl:template>

<xsl:template match="phc:AST_interface_def_list">
</xsl:template>

<xsl:template match="phc:AST_class_def_list">
  <xsl:apply-templates select="phc:AST_class_def" />
</xsl:template>

<xsl:template match="phc:AST_class_def">
                                                                  
# Do not edit this file.                                          
# This file has been generated by phc_to_past.xsl
                                                                  
.sub plumhead :main                                                     
  load_bytecode 'PAST.pbc'                                        
  load_bytecode 'languages/punie/lib/POST.pir'                    
  load_bytecode 'languages/punie/lib/OSTGrammar.pir'              
  load_bytecode 'MIME/Base64.pbc'              
                                                                  
  .local pmc enc_sub, dec_sub
  dec_sub = get_global [ "MIME"; "Base64" ], 'decode_base64'
    
  .sym pmc stmts                                                  
  stmts = new 'PAST::Stmts'                                       
                                                                  
  .sym pmc reg_temp                                               
                                                                  
  .sym pmc reg_expression_stmt                                    
  .sym pmc reg_expression_topexp                                  
  .sym pmc reg_print_op                                           
  .sym pmc reg_expression_exp                                     
                                                                  
  <xsl:apply-templates select="phc:AST_member_list" />

  # say 'AST tree dump:'                                          
  # stmts.dump()                                                  
                                                                  
  # Compile the abstract syntax tree                              
  # down to an opcode syntax tree                                 
  .sym string ost_tg_src                                          
  .sym pmc tge_compiler                                           
  .sym pmc ost_grammar, ost_builder, ost                          
  tge_compiler = new 'TGE::Compiler'                              
  ost_tg_src = _slurp_file('languages/punie/lib/OSTGrammar.tg')   
  ost_grammar = tge_compiler.'compile'(ost_tg_src)                
  ost_builder = ost_grammar.apply(stmts)                          
  ost = ost_builder.get('result')                                 
  unless ost goto ERR_NO_OST                                      
                                                                  
  # Compile the OST down to PIR                                   
  .sym string pir_tg_src                                          
  .sym pmc pir_grammar, pir_builder, pir                          
  pir_tg_src = _slurp_file('languages/punie/lib/PIRGrammar.tg')   
  pir_grammar = tge_compiler.'compile'(pir_tg_src)                
  pir_builder = pir_grammar.apply(ost)                            
  pir = pir_builder.get('result')                                 
  unless pir goto ERR_NO_PIR                                      
                                                                  
  # execute                                                       
  .sym pmc pir_compiler, pir_compiled                             
  pir_compiler = compreg 'PIR'                                    
  pir_compiled = pir_compiler( pir )                              
  pir_compiled()                                                  
  say ''                                                          
  goto CLEANUP                                                    
                                                                  
  ERR_NO_OST:                                                     
    say 'Unable to construct OST.'                                
    goto CLEANUP                                                  
                                                                  
  ERR_NO_PIR:                                                     
    say 'Unable to construct PIR.'                                
    goto CLEANUP                                                  
                                                                  
  CLEANUP:                                                        
.end                                                              
                                                                  
</xsl:template>

<xsl:template match="phc:AST_member_list">
  <xsl:apply-templates select="phc:AST_method" />
</xsl:template>

<xsl:template match="phc:AST_method">
  <xsl:apply-templates select="phc:AST_statement_list" />
</xsl:template>

<xsl:template match="phc:AST_statement_list">
  <xsl:apply-templates select="phc:AST_eval_expr" />
</xsl:template>

<xsl:template match="phc:AST_eval_expr">
  <xsl:apply-templates select="phc:AST_method_invocation" />
</xsl:template>

<xsl:template match="phc:AST_method_invocation">
  <xsl:apply-templates select="phc:AST_actual_parameter_list" />
</xsl:template>

<xsl:template match="phc:AST_actual_parameter_list">
  <xsl:apply-templates select="phc:AST_actual_parameter" />
</xsl:template>

<xsl:template match="phc:AST_actual_parameter">
  <xsl:apply-templates select="phc:Token_string" />
</xsl:template>

<xsl:template match="phc:Token_string">
  <xsl:apply-templates select="phc:value" />
</xsl:template>

<xsl:template match="phc:value">
                                                                  
  # entering 'PRINT node'                                         
    reg_expression_stmt = new 'PAST::Stmt'                        
      reg_expression_topexp = new 'PAST::Exp'                     
        reg_print_op = new 'PAST::Op'                             
        reg_print_op.'op'( 'print' )                              
          reg_expression_exp = new 'PAST::Exp'                    
                                                                   
# entering 'STRING'                                                
            .sym string decoded
            decoded = dec_sub( "<xsl:value-of select="." />" )
            decoded = escape decoded
            reg_temp = new 'PAST::Val'                             
            reg_temp.value( decoded ) 
            reg_temp.valtype( 'strqq' )                            
          reg_expression_exp.'add_child'( reg_temp )         
          null reg_temp                                            
# leaving 'STRING'                                                 
                                                                   
      reg_print_op.'add_child'( reg_expression_exp )               
      reg_expression_topexp.'add_child'( reg_print_op      )       
    reg_expression_stmt.'add_child'( reg_expression_topexp )       
  stmts.'add_child'( reg_expression_stmt )      
  # leaving 'PRINT node'                                           
</xsl:template>

<xsl:template match="/">
  <xsl:apply-templates/>
</xsl:template>

</xsl:stylesheet>
