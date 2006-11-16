<?xml version='1.0'?>
<xsl:stylesheet 
  xmlns:phc="http://www.phpcompiler.org/phc-1.0"
  xmlns:past="http://www.parrotcode.org/PAST-0.1"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0" >
<!--

$Id$

This transformation takes a XML abstract syntax tree as generated 
by phc from PHP source. It generates an XML of the kind of PAST
that is used by punie.

-->
<xsl:output method='xml' indent='yes' />

<xsl:template match="/">
  <xsl:apply-templates select="phc:AST_php_script" />
</xsl:template>

<xsl:template match="phc:AST_php_script" >
  <xsl:apply-templates select="phc:AST_class_def_list" />
</xsl:template>

<xsl:template match="phc:AST_class_def_list" >
  <xsl:apply-templates select="phc:AST_class_def" />
</xsl:template>

<xsl:template match="phc:AST_class_def" >
</xsl:template>
<xsl:template match="phc:AST_class_def[phc:Token_class_name[phc:value='%MAIN%']]">
  <xsl:apply-templates select="phc:AST_member_list" />
</xsl:template>

<xsl:template match="phc:AST_member_list" >
  <xsl:apply-templates select="phc:AST_method" />
</xsl:template>

<xsl:template match="phc:AST_method" >
</xsl:template>
<xsl:template match="phc:AST_method[phc:AST_signature[phc:Token_method_name[phc:value='%run%']]]">
  <past:Stmts>
    <xsl:apply-templates select="phc:AST_statement_list" />
  </past:Stmts>                                                                  
</xsl:template>

<xsl:template match="phc:AST_statement_list" >
  <xsl:apply-templates select="phc:AST_eval_expr" />
  <xsl:apply-templates select="phc:AST_if" />
</xsl:template>

<xsl:template match="phc:AST_eval_expr" >
  <xsl:apply-templates select="phc:AST_method_invocation" />
  <xsl:apply-templates select="phc:AST_assignment" />
</xsl:template>

<xsl:template match="phc:AST_method_invocation" >
  <past:Op name='print' >
    <xsl:apply-templates select="phc:AST_actual_parameter_list" />
  </past:Op>
</xsl:template>

<xsl:template match="phc:AST_actual_parameter_list" >
  <xsl:apply-templates select="phc:AST_actual_parameter" />
</xsl:template>

<xsl:template match="phc:AST_actual_parameter" >
  <xsl:apply-templates select="phc:Token_string | phc:Token_int | phc:Token_real | phc:AST_bin_op | phc:AST_variable" />
</xsl:template>

<xsl:template match="phc:AST_bin_op" >
  <past:Op op="infix:+" >
    <xsl:attribute name="op" >infix:<xsl:value-of select="phc:Token_op/phc:value" /></xsl:attribute>
    <xsl:apply-templates select="phc:Token_int | phc:Token_real | phc:AST_bin_op" />
  </past:Op>
</xsl:template>

<xsl:template match="phc:AST_assignment" >
  <past:Op op='infix:=' >
    <xsl:apply-templates select="phc:AST_variable" />
    <xsl:apply-templates select="phc:Token_string" />
  </past:Op>
</xsl:template>

<xsl:template match="phc:AST_variable" >
  <past:Var vartype="scalar" scope="global" >
    <xsl:attribute name="varname" ><xsl:value-of select="phc:Token_variable_name/phc:value" /></xsl:attribute>
  </past:Var>
</xsl:template>

<xsl:template match="phc:Token_string" >
  <past:Val ctype="s" vtype=".Perl6Str" >
    <xsl:attribute name="encoding" ><xsl:value-of select="phc:value/@encoding" /></xsl:attribute>
    <xsl:attribute name="name" ><xsl:value-of select="phc:value" /></xsl:attribute>
  </past:Val>
</xsl:template>

<xsl:template match="phc:Token_int" >
  <past:Val valtype="int" >
    <xsl:apply-templates select="phc:value" />
  </past:Val>
</xsl:template>

<!-- looks like phc is running into a floating point issue -->
<xsl:template match="phc:Token_real" >
  <past:Val valtype="num" >
    <xsl:apply-templates select="phc:source_rep" />
  </past:Val>
</xsl:template>

<xsl:template match="phc:value | phc:source_rep" >
  <xsl:value-of select="." />
</xsl:template>

<xsl:template match="phc:AST_if" >
  <past:Stmt>
    <past:Op op="print" >
      <past:Val valtype="strq" encoding="base64" >ICAgIDxzdHJvbmc+Q29uZGl0aW9uIGlzIHRydWUuPC9zdHJvbmc+CiAg</past:Val>
    </past:Op>
  </past:Stmt>
</xsl:template>

</xsl:stylesheet>
