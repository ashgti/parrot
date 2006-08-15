// $ANTLR 3.0b3 grammar/antlr_3/bc.g 2006-08-15 13:19:31

import org.antlr.runtime.*;
import java.util.Stack;
import java.util.List;
import java.util.ArrayList;


import org.antlr.runtime.tree.*;

public class BcParser extends Parser {
    public static final String[] tokenNames = new String[] {
        "<invalid>", "<EOR>", "<DOWN>", "<UP>", "PRINT", "PROGRAM", "SAY", "UNARY_MINUS", "VAR", "Quit", "NEWLINE", "SEMICOLON", "ASSIGN_OP", "STRING", "LETTER", "PLUS", "MINUS", "MUL_OP", "NUMBER", "INTEGER", "INCR_DECR", "ML_COMMENT", "WS", "'('", "')'"
    };
    public static final int INCR_DECR=20;
    public static final int LETTER=14;
    public static final int MINUS=16;
    public static final int WS=22;
    public static final int NUMBER=18;
    public static final int STRING=13;
    public static final int MUL_OP=17;
    public static final int Quit=9;
    public static final int PROGRAM=5;
    public static final int NEWLINE=10;
    public static final int VAR=8;
    public static final int ASSIGN_OP=12;
    public static final int INTEGER=19;
    public static final int SAY=6;
    public static final int PRINT=4;
    public static final int UNARY_MINUS=7;
    public static final int SEMICOLON=11;
    public static final int EOF=-1;
    public static final int ML_COMMENT=21;
    public static final int PLUS=15;

        public BcParser(TokenStream input) {
            super(input);
        }
        
    protected TreeAdaptor adaptor = new CommonTreeAdaptor();

    public void setTreeAdaptor(TreeAdaptor adaptor) {
        this.adaptor = adaptor;
    }
    public TreeAdaptor getTreeAdaptor() {
        return adaptor;
    }

    public String[] getTokenNames() { return tokenNames; }
    public String getGrammarFileName() { return "grammar/antlr_3/bc.g"; }


    public static class program_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start program
    // grammar/antlr_3/bc.g:30:1: program : ( input_item )+ Quit NEWLINE -> ^( PROGRAM ( input_item )+ ) ;
    public program_return program() throws RecognitionException {   
        program_return retval = new program_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token Quit2=null;
        Token NEWLINE3=null;
        input_item_return input_item1 = null;

        List list_input_item=new ArrayList();
        List list_Quit=new ArrayList();
        List list_NEWLINE=new ArrayList();
        CommonTree Quit2_tree=null;
        CommonTree NEWLINE3_tree=null;

        try {
            // grammar/antlr_3/bc.g:31:5: ( ( input_item )+ Quit NEWLINE -> ^( PROGRAM ( input_item )+ ) )
            // grammar/antlr_3/bc.g:31:5: ( input_item )+ Quit NEWLINE
            {
            // grammar/antlr_3/bc.g:31:5: ( input_item )+
            int cnt1=0;
            loop1:
            do {
                int alt1=2;
                int LA1_0 = input.LA(1);
                if ( ((LA1_0>=NEWLINE && LA1_0<=SEMICOLON)||(LA1_0>=STRING && LA1_0<=LETTER)||LA1_0==MINUS||LA1_0==NUMBER||LA1_0==23) ) {
                    alt1=1;
                }


                switch (alt1) {
            	case 1 :
            	    // grammar/antlr_3/bc.g:31:5: input_item
            	    {
            	    pushFollow(FOLLOW_input_item_in_program105);
            	    input_item1=input_item();
            	    _fsp--;

            	    list_input_item.add(input_item1.tree);

            	    }
            	    break;

            	default :
            	    if ( cnt1 >= 1 ) break loop1;
                        EarlyExitException eee =
                            new EarlyExitException(1, input);
                        throw eee;
                }
                cnt1++;
            } while (true);

            Quit2=(Token)input.LT(1);
            match(input,Quit,FOLLOW_Quit_in_program108); 
            list_Quit.add(Quit2);

            NEWLINE3=(Token)input.LT(1);
            match(input,NEWLINE,FOLLOW_NEWLINE_in_program110); 
            list_NEWLINE.add(NEWLINE3);


            // AST REWRITE
            int i_0 = 0;
            retval.tree = root_0;
            root_0 = (CommonTree)adaptor.nil();
            // 31:30: -> ^( PROGRAM ( input_item )+ )
            {
                // grammar/antlr_3/bc.g:31:33: ^( PROGRAM ( input_item )+ )
                {
                CommonTree root_1 = (CommonTree)adaptor.nil();
                root_1 = (CommonTree)adaptor.becomeRoot(adaptor.create(PROGRAM, "PROGRAM"), root_1);

                // grammar/antlr_3/bc.g:31:44: ( input_item )+
                {
                int n_1 = list_input_item == null ? 0 : list_input_item.size();
                 


                if ( n_1==0 ) throw new RuntimeException("Must have more than one element for (...)+ loops");
                for (int i_1=0; i_1<n_1; i_1++) {
                    adaptor.addChild(root_1, list_input_item.get(i_1));

                }
                }

                adaptor.addChild(root_0, root_1);
                }

            }



            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end program

    public static class input_item_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start input_item
    // grammar/antlr_3/bc.g:34:1: input_item : semicolon_list NEWLINE! ;
    public input_item_return input_item() throws RecognitionException {   
        input_item_return retval = new input_item_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token NEWLINE5=null;
        semicolon_list_return semicolon_list4 = null;


        CommonTree NEWLINE5_tree=null;

        try {
            // grammar/antlr_3/bc.g:35:5: ( semicolon_list NEWLINE! )
            // grammar/antlr_3/bc.g:35:5: semicolon_list NEWLINE!
            {
            root_0 = (CommonTree)adaptor.nil();

            pushFollow(FOLLOW_semicolon_list_in_input_item134);
            semicolon_list4=semicolon_list();
            _fsp--;

            adaptor.addChild(root_0, semicolon_list4.tree);
            NEWLINE5=(Token)input.LT(1);
            match(input,NEWLINE,FOLLOW_NEWLINE_in_input_item136); 

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end input_item

    public static class semicolon_list_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start semicolon_list
    // grammar/antlr_3/bc.g:38:1: semicolon_list : ( statement )? ( ( SEMICOLON! )+ ( statement )? )* ;
    public semicolon_list_return semicolon_list() throws RecognitionException {   
        semicolon_list_return retval = new semicolon_list_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token SEMICOLON7=null;
        statement_return statement6 = null;

        statement_return statement8 = null;


        CommonTree SEMICOLON7_tree=null;

        try {
            // grammar/antlr_3/bc.g:39:5: ( ( statement )? ( ( SEMICOLON! )+ ( statement )? )* )
            // grammar/antlr_3/bc.g:39:5: ( statement )? ( ( SEMICOLON! )+ ( statement )? )*
            {
            root_0 = (CommonTree)adaptor.nil();

            // grammar/antlr_3/bc.g:39:5: ( statement )?
            int alt2=2;
            int LA2_0 = input.LA(1);
            if ( ((LA2_0>=STRING && LA2_0<=LETTER)||LA2_0==MINUS||LA2_0==NUMBER||LA2_0==23) ) {
                alt2=1;
            }
            switch (alt2) {
                case 1 :
                    // grammar/antlr_3/bc.g:39:5: statement
                    {
                    CommonTree root_1 = (CommonTree)adaptor.nil();

                    pushFollow(FOLLOW_statement_in_semicolon_list151);
                    statement6=statement();
                    _fsp--;

                    adaptor.addChild(root_1, statement6.tree);

                    adaptor.addChild(root_0, root_1);

                    }
                    break;

            }

            // grammar/antlr_3/bc.g:39:16: ( ( SEMICOLON! )+ ( statement )? )*
            loop5:
            do {
                int alt5=2;
                int LA5_0 = input.LA(1);
                if ( (LA5_0==SEMICOLON) ) {
                    alt5=1;
                }


                switch (alt5) {
            	case 1 :
            	    // grammar/antlr_3/bc.g:39:18: ( SEMICOLON! )+ ( statement )?
            	    {
            	    CommonTree root_1 = (CommonTree)adaptor.nil();

            	    // grammar/antlr_3/bc.g:39:18: ( SEMICOLON! )+
            	    int cnt3=0;
            	    loop3:
            	    do {
            	        int alt3=2;
            	        int LA3_0 = input.LA(1);
            	        if ( (LA3_0==SEMICOLON) ) {
            	            alt3=1;
            	        }


            	        switch (alt3) {
            	    	case 1 :
            	    	    // grammar/antlr_3/bc.g:39:19: SEMICOLON!
            	    	    {
            	    	    CommonTree root_2 = (CommonTree)adaptor.nil();

            	    	    SEMICOLON7=(Token)input.LT(1);
            	    	    match(input,SEMICOLON,FOLLOW_SEMICOLON_in_semicolon_list157); 

            	    	    adaptor.addChild(root_1, root_2);

            	    	    }
            	    	    break;

            	    	default :
            	    	    if ( cnt3 >= 1 ) break loop3;
            	                EarlyExitException eee =
            	                    new EarlyExitException(3, input);
            	                throw eee;
            	        }
            	        cnt3++;
            	    } while (true);

            	    // grammar/antlr_3/bc.g:39:32: ( statement )?
            	    int alt4=2;
            	    int LA4_0 = input.LA(1);
            	    if ( ((LA4_0>=STRING && LA4_0<=LETTER)||LA4_0==MINUS||LA4_0==NUMBER||LA4_0==23) ) {
            	        alt4=1;
            	    }
            	    switch (alt4) {
            	        case 1 :
            	            // grammar/antlr_3/bc.g:39:32: statement
            	            {
            	            CommonTree root_2 = (CommonTree)adaptor.nil();

            	            pushFollow(FOLLOW_statement_in_semicolon_list162);
            	            statement8=statement();
            	            _fsp--;

            	            adaptor.addChild(root_2, statement8.tree);

            	            adaptor.addChild(root_1, root_2);

            	            }
            	            break;

            	    }


            	    adaptor.addChild(root_0, root_1);

            	    }
            	    break;

            	default :
            	    break loop5;
                }
            } while (true);


            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end semicolon_list

    public static class statement_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start statement
    // grammar/antlr_3/bc.g:43:1: statement : ( named_expression ASSIGN_OP^^ expression | expression -> ^( SAY expression ) | STRING -> ^( PRINT STRING ) );
    public statement_return statement() throws RecognitionException {   
        statement_return retval = new statement_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token ASSIGN_OP10=null;
        Token STRING13=null;
        named_expression_return named_expression9 = null;

        expression_return expression11 = null;

        expression_return expression12 = null;

        List list_expression=new ArrayList();
        List list_STRING=new ArrayList();
        CommonTree ASSIGN_OP10_tree=null;
        CommonTree STRING13_tree=null;

        try {
            // grammar/antlr_3/bc.g:44:5: ( named_expression ASSIGN_OP^^ expression | expression -> ^( SAY expression ) | STRING -> ^( PRINT STRING ) )
            int alt6=3;
            switch ( input.LA(1) ) {
            case LETTER:
                int LA6_1 = input.LA(2);
                if ( (LA6_1==ASSIGN_OP) ) {
                    alt6=1;
                }
                else if ( ((LA6_1>=NEWLINE && LA6_1<=SEMICOLON)||(LA6_1>=PLUS && LA6_1<=MUL_OP)) ) {
                    alt6=2;
                }
                else {
                    NoViableAltException nvae =
                        new NoViableAltException("43:1: statement : ( named_expression ASSIGN_OP^^ expression | expression -> ^( SAY expression ) | STRING -> ^( PRINT STRING ) );", 6, 1, input);

                    throw nvae;
                }
                break;
            case MINUS:
            case NUMBER:
            case 23:
                alt6=2;
                break;
            case STRING:
                alt6=3;
                break;
            default:
                NoViableAltException nvae =
                    new NoViableAltException("43:1: statement : ( named_expression ASSIGN_OP^^ expression | expression -> ^( SAY expression ) | STRING -> ^( PRINT STRING ) );", 6, 0, input);

                throw nvae;
            }

            switch (alt6) {
                case 1 :
                    // grammar/antlr_3/bc.g:44:5: named_expression ASSIGN_OP^^ expression
                    {
                    root_0 = (CommonTree)adaptor.nil();

                    pushFollow(FOLLOW_named_expression_in_statement180);
                    named_expression9=named_expression();
                    _fsp--;

                    adaptor.addChild(root_0, named_expression9.tree);
                    ASSIGN_OP10=(Token)input.LT(1);
                    match(input,ASSIGN_OP,FOLLOW_ASSIGN_OP_in_statement182); 
                    ASSIGN_OP10_tree = (CommonTree)adaptor.create(ASSIGN_OP10);
                    root_0 = (CommonTree)adaptor.becomeRoot(ASSIGN_OP10_tree, root_0);

                    pushFollow(FOLLOW_expression_in_statement185);
                    expression11=expression();
                    _fsp--;

                    adaptor.addChild(root_0, expression11.tree);

                    }
                    break;
                case 2 :
                    // grammar/antlr_3/bc.g:46:5: expression
                    {
                    pushFollow(FOLLOW_expression_in_statement197);
                    expression12=expression();
                    _fsp--;

                    list_expression.add(expression12.tree);

                    // AST REWRITE
                    int i_0 = 0;
                    retval.tree = root_0;
                    root_0 = (CommonTree)adaptor.nil();
                    // 46:16: -> ^( SAY expression )
                    {
                        // grammar/antlr_3/bc.g:46:19: ^( SAY expression )
                        {
                        CommonTree root_1 = (CommonTree)adaptor.nil();
                        root_1 = (CommonTree)adaptor.becomeRoot(adaptor.create(SAY, "SAY"), root_1);

                        adaptor.addChild(root_1, list_expression.get(i_0));

                        adaptor.addChild(root_0, root_1);
                        }

                    }



                    }
                    break;
                case 3 :
                    // grammar/antlr_3/bc.g:48:5: STRING
                    {
                    STRING13=(Token)input.LT(1);
                    match(input,STRING,FOLLOW_STRING_in_statement219); 
                    list_STRING.add(STRING13);


                    // AST REWRITE
                    int i_0 = 0;
                    retval.tree = root_0;
                    root_0 = (CommonTree)adaptor.nil();
                    // 48:12: -> ^( PRINT STRING )
                    {
                        // grammar/antlr_3/bc.g:48:15: ^( PRINT STRING )
                        {
                        CommonTree root_1 = (CommonTree)adaptor.nil();
                        root_1 = (CommonTree)adaptor.becomeRoot(adaptor.create(PRINT, "PRINT"), root_1);

                        adaptor.addChild(root_1, (Token)list_STRING.get(i_0));

                        adaptor.addChild(root_0, root_1);
                        }

                    }



                    }
                    break;

            }
        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end statement

    public static class expression_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start expression
    // grammar/antlr_3/bc.g:51:1: expression : adding_expression ;
    public expression_return expression() throws RecognitionException {   
        expression_return retval = new expression_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        adding_expression_return adding_expression14 = null;



        try {
            // grammar/antlr_3/bc.g:52:5: ( adding_expression )
            // grammar/antlr_3/bc.g:52:5: adding_expression
            {
            root_0 = (CommonTree)adaptor.nil();

            pushFollow(FOLLOW_adding_expression_in_expression242);
            adding_expression14=adding_expression();
            _fsp--;

            adaptor.addChild(root_0, adding_expression14.tree);

            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end expression

    public static class named_expression_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start named_expression
    // grammar/antlr_3/bc.g:55:1: named_expression : LETTER -> ^( VAR LETTER ) ;
    public named_expression_return named_expression() throws RecognitionException {   
        named_expression_return retval = new named_expression_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token LETTER15=null;
        List list_LETTER=new ArrayList();
        CommonTree LETTER15_tree=null;

        try {
            // grammar/antlr_3/bc.g:56:5: ( LETTER -> ^( VAR LETTER ) )
            // grammar/antlr_3/bc.g:56:5: LETTER
            {
            LETTER15=(Token)input.LT(1);
            match(input,LETTER,FOLLOW_LETTER_in_named_expression255); 
            list_LETTER.add(LETTER15);


            // AST REWRITE
            int i_0 = 0;
            retval.tree = root_0;
            root_0 = (CommonTree)adaptor.nil();
            // 56:12: -> ^( VAR LETTER )
            {
                // grammar/antlr_3/bc.g:56:15: ^( VAR LETTER )
                {
                CommonTree root_1 = (CommonTree)adaptor.nil();
                root_1 = (CommonTree)adaptor.becomeRoot(adaptor.create(VAR, "VAR"), root_1);

                adaptor.addChild(root_1, (Token)list_LETTER.get(i_0));

                adaptor.addChild(root_0, root_1);
                }

            }



            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end named_expression

    public static class adding_expression_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start adding_expression
    // grammar/antlr_3/bc.g:60:1: adding_expression : multiplying_expression ( ( PLUS^^ | MINUS^^ ) multiplying_expression )* ;
    public adding_expression_return adding_expression() throws RecognitionException {   
        adding_expression_return retval = new adding_expression_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token PLUS17=null;
        Token MINUS18=null;
        multiplying_expression_return multiplying_expression16 = null;

        multiplying_expression_return multiplying_expression19 = null;


        CommonTree PLUS17_tree=null;
        CommonTree MINUS18_tree=null;

        try {
            // grammar/antlr_3/bc.g:61:5: ( multiplying_expression ( ( PLUS^^ | MINUS^^ ) multiplying_expression )* )
            // grammar/antlr_3/bc.g:61:5: multiplying_expression ( ( PLUS^^ | MINUS^^ ) multiplying_expression )*
            {
            root_0 = (CommonTree)adaptor.nil();

            pushFollow(FOLLOW_multiplying_expression_in_adding_expression280);
            multiplying_expression16=multiplying_expression();
            _fsp--;

            adaptor.addChild(root_0, multiplying_expression16.tree);
            // grammar/antlr_3/bc.g:61:28: ( ( PLUS^^ | MINUS^^ ) multiplying_expression )*
            loop8:
            do {
                int alt8=2;
                int LA8_0 = input.LA(1);
                if ( ((LA8_0>=PLUS && LA8_0<=MINUS)) ) {
                    alt8=1;
                }


                switch (alt8) {
            	case 1 :
            	    // grammar/antlr_3/bc.g:61:30: ( PLUS^^ | MINUS^^ ) multiplying_expression
            	    {
            	    CommonTree root_1 = (CommonTree)adaptor.nil();

            	    // grammar/antlr_3/bc.g:61:30: ( PLUS^^ | MINUS^^ )
            	    int alt7=2;
            	    int LA7_0 = input.LA(1);
            	    if ( (LA7_0==PLUS) ) {
            	        alt7=1;
            	    }
            	    else if ( (LA7_0==MINUS) ) {
            	        alt7=2;
            	    }
            	    else {
            	        NoViableAltException nvae =
            	            new NoViableAltException("61:30: ( PLUS^^ | MINUS^^ )", 7, 0, input);

            	        throw nvae;
            	    }
            	    switch (alt7) {
            	        case 1 :
            	            // grammar/antlr_3/bc.g:61:32: PLUS^^
            	            {
            	            CommonTree root_2 = (CommonTree)adaptor.nil();

            	            PLUS17=(Token)input.LT(1);
            	            match(input,PLUS,FOLLOW_PLUS_in_adding_expression286); 
            	            PLUS17_tree = (CommonTree)adaptor.create(PLUS17);
            	            root_0 = (CommonTree)adaptor.becomeRoot(PLUS17_tree, root_0);


            	            adaptor.addChild(root_1, root_2);

            	            }
            	            break;
            	        case 2 :
            	            // grammar/antlr_3/bc.g:61:41: MINUS^^
            	            {
            	            CommonTree root_2 = (CommonTree)adaptor.nil();

            	            MINUS18=(Token)input.LT(1);
            	            match(input,MINUS,FOLLOW_MINUS_in_adding_expression291); 
            	            MINUS18_tree = (CommonTree)adaptor.create(MINUS18);
            	            root_0 = (CommonTree)adaptor.becomeRoot(MINUS18_tree, root_0);


            	            adaptor.addChild(root_1, root_2);

            	            }
            	            break;

            	    }

            	    pushFollow(FOLLOW_multiplying_expression_in_adding_expression296);
            	    multiplying_expression19=multiplying_expression();
            	    _fsp--;

            	    adaptor.addChild(root_1, multiplying_expression19.tree);

            	    adaptor.addChild(root_0, root_1);

            	    }
            	    break;

            	default :
            	    break loop8;
                }
            } while (true);


            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end adding_expression

    public static class multiplying_expression_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start multiplying_expression
    // grammar/antlr_3/bc.g:64:1: multiplying_expression : unary_expression ( MUL_OP^^ unary_expression )* ;
    public multiplying_expression_return multiplying_expression() throws RecognitionException {   
        multiplying_expression_return retval = new multiplying_expression_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token MUL_OP21=null;
        unary_expression_return unary_expression20 = null;

        unary_expression_return unary_expression22 = null;


        CommonTree MUL_OP21_tree=null;

        try {
            // grammar/antlr_3/bc.g:65:5: ( unary_expression ( MUL_OP^^ unary_expression )* )
            // grammar/antlr_3/bc.g:65:5: unary_expression ( MUL_OP^^ unary_expression )*
            {
            root_0 = (CommonTree)adaptor.nil();

            pushFollow(FOLLOW_unary_expression_in_multiplying_expression312);
            unary_expression20=unary_expression();
            _fsp--;

            adaptor.addChild(root_0, unary_expression20.tree);
            // grammar/antlr_3/bc.g:65:22: ( MUL_OP^^ unary_expression )*
            loop9:
            do {
                int alt9=2;
                int LA9_0 = input.LA(1);
                if ( (LA9_0==MUL_OP) ) {
                    alt9=1;
                }


                switch (alt9) {
            	case 1 :
            	    // grammar/antlr_3/bc.g:65:24: MUL_OP^^ unary_expression
            	    {
            	    CommonTree root_1 = (CommonTree)adaptor.nil();

            	    MUL_OP21=(Token)input.LT(1);
            	    match(input,MUL_OP,FOLLOW_MUL_OP_in_multiplying_expression316); 
            	    MUL_OP21_tree = (CommonTree)adaptor.create(MUL_OP21);
            	    root_0 = (CommonTree)adaptor.becomeRoot(MUL_OP21_tree, root_0);

            	    pushFollow(FOLLOW_unary_expression_in_multiplying_expression319);
            	    unary_expression22=unary_expression();
            	    _fsp--;

            	    adaptor.addChild(root_1, unary_expression22.tree);

            	    adaptor.addChild(root_0, root_1);

            	    }
            	    break;

            	default :
            	    break loop9;
                }
            } while (true);


            }

        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end multiplying_expression

    public static class unary_expression_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start unary_expression
    // grammar/antlr_3/bc.g:68:1: unary_expression : ( postfix_expression | MINUS postfix_expression -> ^( UNARY_MINUS postfix_expression ) );
    public unary_expression_return unary_expression() throws RecognitionException {   
        unary_expression_return retval = new unary_expression_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token MINUS24=null;
        postfix_expression_return postfix_expression23 = null;

        postfix_expression_return postfix_expression25 = null;

        List list_postfix_expression=new ArrayList();
        List list_MINUS=new ArrayList();
        CommonTree MINUS24_tree=null;

        try {
            // grammar/antlr_3/bc.g:69:5: ( postfix_expression | MINUS postfix_expression -> ^( UNARY_MINUS postfix_expression ) )
            int alt10=2;
            int LA10_0 = input.LA(1);
            if ( (LA10_0==LETTER||LA10_0==NUMBER||LA10_0==23) ) {
                alt10=1;
            }
            else if ( (LA10_0==MINUS) ) {
                alt10=2;
            }
            else {
                NoViableAltException nvae =
                    new NoViableAltException("68:1: unary_expression : ( postfix_expression | MINUS postfix_expression -> ^( UNARY_MINUS postfix_expression ) );", 10, 0, input);

                throw nvae;
            }
            switch (alt10) {
                case 1 :
                    // grammar/antlr_3/bc.g:69:5: postfix_expression
                    {
                    root_0 = (CommonTree)adaptor.nil();

                    pushFollow(FOLLOW_postfix_expression_in_unary_expression335);
                    postfix_expression23=postfix_expression();
                    _fsp--;

                    adaptor.addChild(root_0, postfix_expression23.tree);

                    }
                    break;
                case 2 :
                    // grammar/antlr_3/bc.g:71:5: MINUS postfix_expression
                    {
                    MINUS24=(Token)input.LT(1);
                    match(input,MINUS,FOLLOW_MINUS_in_unary_expression347); 
                    list_MINUS.add(MINUS24);

                    pushFollow(FOLLOW_postfix_expression_in_unary_expression349);
                    postfix_expression25=postfix_expression();
                    _fsp--;

                    list_postfix_expression.add(postfix_expression25.tree);

                    // AST REWRITE
                    int i_0 = 0;
                    retval.tree = root_0;
                    root_0 = (CommonTree)adaptor.nil();
                    // 71:30: -> ^( UNARY_MINUS postfix_expression )
                    {
                        // grammar/antlr_3/bc.g:71:33: ^( UNARY_MINUS postfix_expression )
                        {
                        CommonTree root_1 = (CommonTree)adaptor.nil();
                        root_1 = (CommonTree)adaptor.becomeRoot(adaptor.create(UNARY_MINUS, "UNARY_MINUS"), root_1);

                        adaptor.addChild(root_1, list_postfix_expression.get(i_0));

                        adaptor.addChild(root_0, root_1);
                        }

                    }



                    }
                    break;

            }
        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end unary_expression

    public static class postfix_expression_return extends ParserRuleReturnScope {
        CommonTree tree;
        public Object getTree() { return tree; }
    };

    // $ANTLR start postfix_expression
    // grammar/antlr_3/bc.g:74:1: postfix_expression : ( NUMBER | named_expression | '(' expression ')' -> expression );
    public postfix_expression_return postfix_expression() throws RecognitionException {   
        postfix_expression_return retval = new postfix_expression_return();
        retval.start = input.LT(1);

        CommonTree root_0 = null;

        Token NUMBER26=null;
        Token char_literal28=null;
        Token char_literal30=null;
        named_expression_return named_expression27 = null;

        expression_return expression29 = null;

        List list_expression=new ArrayList();
        List list_24=new ArrayList();
        List list_23=new ArrayList();
        CommonTree NUMBER26_tree=null;
        CommonTree char_literal28_tree=null;
        CommonTree char_literal30_tree=null;

        try {
            // grammar/antlr_3/bc.g:75:5: ( NUMBER | named_expression | '(' expression ')' -> expression )
            int alt11=3;
            switch ( input.LA(1) ) {
            case NUMBER:
                alt11=1;
                break;
            case LETTER:
                alt11=2;
                break;
            case 23:
                alt11=3;
                break;
            default:
                NoViableAltException nvae =
                    new NoViableAltException("74:1: postfix_expression : ( NUMBER | named_expression | '(' expression ')' -> expression );", 11, 0, input);

                throw nvae;
            }

            switch (alt11) {
                case 1 :
                    // grammar/antlr_3/bc.g:75:5: NUMBER
                    {
                    root_0 = (CommonTree)adaptor.nil();

                    NUMBER26=(Token)input.LT(1);
                    match(input,NUMBER,FOLLOW_NUMBER_in_postfix_expression372); 
                    NUMBER26_tree = (CommonTree)adaptor.create(NUMBER26);
                    adaptor.addChild(root_0, NUMBER26_tree);


                    }
                    break;
                case 2 :
                    // grammar/antlr_3/bc.g:77:5: named_expression
                    {
                    root_0 = (CommonTree)adaptor.nil();

                    pushFollow(FOLLOW_named_expression_in_postfix_expression384);
                    named_expression27=named_expression();
                    _fsp--;

                    adaptor.addChild(root_0, named_expression27.tree);

                    }
                    break;
                case 3 :
                    // grammar/antlr_3/bc.g:79:5: '(' expression ')'
                    {
                    char_literal28=(Token)input.LT(1);
                    match(input,23,FOLLOW_23_in_postfix_expression401); 
                    list_23.add(char_literal28);

                    pushFollow(FOLLOW_expression_in_postfix_expression403);
                    expression29=expression();
                    _fsp--;

                    list_expression.add(expression29.tree);
                    char_literal30=(Token)input.LT(1);
                    match(input,24,FOLLOW_24_in_postfix_expression405); 
                    list_24.add(char_literal30);


                    // AST REWRITE
                    int i_0 = 0;
                    retval.tree = root_0;
                    root_0 = (CommonTree)adaptor.nil();
                    // 79:24: -> expression
                    {
                        adaptor.addChild(root_0, list_expression.get(i_0));

                    }



                    }
                    break;

            }
        }
        catch (RecognitionException re) {
            reportError(re);
            recover(input,re);
        }
        finally {
            retval.stop = input.LT(-1);

                retval.tree = (CommonTree)adaptor.rulePostProcessing(root_0);
                adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

        }
        return retval;
    }
    // $ANTLR end postfix_expression


 

    public static final BitSet FOLLOW_input_item_in_program105 = new BitSet(new long[]{0x0000000000856E00L});
    public static final BitSet FOLLOW_Quit_in_program108 = new BitSet(new long[]{0x0000000000000400L});
    public static final BitSet FOLLOW_NEWLINE_in_program110 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_semicolon_list_in_input_item134 = new BitSet(new long[]{0x0000000000000400L});
    public static final BitSet FOLLOW_NEWLINE_in_input_item136 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_statement_in_semicolon_list151 = new BitSet(new long[]{0x0000000000000802L});
    public static final BitSet FOLLOW_SEMICOLON_in_semicolon_list157 = new BitSet(new long[]{0x0000000000856802L});
    public static final BitSet FOLLOW_statement_in_semicolon_list162 = new BitSet(new long[]{0x0000000000000802L});
    public static final BitSet FOLLOW_named_expression_in_statement180 = new BitSet(new long[]{0x0000000000001000L});
    public static final BitSet FOLLOW_ASSIGN_OP_in_statement182 = new BitSet(new long[]{0x0000000000854000L});
    public static final BitSet FOLLOW_expression_in_statement185 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_expression_in_statement197 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_STRING_in_statement219 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_adding_expression_in_expression242 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_LETTER_in_named_expression255 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_multiplying_expression_in_adding_expression280 = new BitSet(new long[]{0x0000000000018002L});
    public static final BitSet FOLLOW_PLUS_in_adding_expression286 = new BitSet(new long[]{0x0000000000854000L});
    public static final BitSet FOLLOW_MINUS_in_adding_expression291 = new BitSet(new long[]{0x0000000000854000L});
    public static final BitSet FOLLOW_multiplying_expression_in_adding_expression296 = new BitSet(new long[]{0x0000000000018002L});
    public static final BitSet FOLLOW_unary_expression_in_multiplying_expression312 = new BitSet(new long[]{0x0000000000020002L});
    public static final BitSet FOLLOW_MUL_OP_in_multiplying_expression316 = new BitSet(new long[]{0x0000000000854000L});
    public static final BitSet FOLLOW_unary_expression_in_multiplying_expression319 = new BitSet(new long[]{0x0000000000020002L});
    public static final BitSet FOLLOW_postfix_expression_in_unary_expression335 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_MINUS_in_unary_expression347 = new BitSet(new long[]{0x0000000000844000L});
    public static final BitSet FOLLOW_postfix_expression_in_unary_expression349 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_NUMBER_in_postfix_expression372 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_named_expression_in_postfix_expression384 = new BitSet(new long[]{0x0000000000000002L});
    public static final BitSet FOLLOW_23_in_postfix_expression401 = new BitSet(new long[]{0x0000000000854000L});
    public static final BitSet FOLLOW_expression_in_postfix_expression403 = new BitSet(new long[]{0x0000000001000000L});
    public static final BitSet FOLLOW_24_in_postfix_expression405 = new BitSet(new long[]{0x0000000000000002L});

}