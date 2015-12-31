#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "token.h"
#include "array.h"
#include "allocator.h"
#include "utils.h"

void lip_lexer_init(
	lip_lexer_t* lexer,
	lip_allocator_t* allocator
)
{
	lexer->allocator = allocator;
	lexer->capture_buff = lip_array_new(allocator);
	lexer->strings = lip_array_new(allocator);
	lip_lexer_reset(lexer, NULL, NULL);
}

static inline void lip_lexer_release_strings(lip_lexer_t* lexer)
{
	lip_array_foreach(char*, string, lexer->strings)
	{
		lip_free(lexer->allocator, *string);
	}
	lip_array_clear(lexer->strings);
}

void lip_lexer_reset(lip_lexer_t* lexer, lip_read_fn_t read_fn, void* read_ctx)
{
	lip_array_clear(lexer->capture_buff);
	lip_lexer_release_strings(lexer);
	lexer->location.line = 1;
	lexer->location.column = 1;
	lexer->buff = 0;
	lexer->capturing = false;
	lexer->buffered = false;
	lexer->eos = false;
	lexer->read_fn = read_fn;
	lexer->read_ctx = read_ctx;
}

void lip_lexer_cleanup(lip_lexer_t* lexer)
{
	lip_lexer_release_strings(lexer);
	lip_array_delete(lexer->capture_buff);
	lip_array_delete(lexer->strings);
}

static inline void lip_lexer_begin_capture(lip_lexer_t* lexer)
{
	lexer->capturing = true;
}

static inline void lip_lexer_reset_capture(lip_lexer_t* lexer)
{
	lip_array_clear(lexer->capture_buff);
	lexer->capturing = false;
}

static inline lip_string_ref_t lip_lexer_end_capture(lip_lexer_t* lexer)
{
	lip_array_push(lexer->capture_buff, 0); // null-terminate
	unsigned int len = lip_array_len(lexer->capture_buff);
	char* string = lip_malloc(lexer->allocator, len);
	memcpy(string, lexer->capture_buff, len);
	lip_string_ref_t ref = { len - 1, string }; // exclude null terminator

	lip_array_push(lexer->strings, string);
	lip_lexer_reset_capture(lexer);

	return ref;
}

static inline lip_lex_status_t lip_lexer_make_token(
	lip_lexer_t* lexer,
	lip_token_t* token,
	lip_token_type_t type
)
{
	token->type = type;
	token->lexeme = lip_lexer_end_capture(lexer);
	token->end = lexer->location;
	--token->end.column;

	return LIP_LEX_OK;
}

static inline bool lip_lexer_peek_char(lip_lexer_t* lexer, char* ch)
{
	if(lexer->buffered) { *ch = lexer->buff; return true; }

	if(lexer->read_fn(&lexer->buff, 1, lexer->read_ctx))
	{
		lexer->buffered = true;
		*ch = lexer->buff;
		return true;
	}
	else
	{
		lexer->eos = true;
		return false;
	}
}

static inline void lip_lexer_consume_char(lip_lexer_t* lexer)
{
	lexer->buffered = false;
	if(lexer->capturing)
	{
		lip_array_push(lexer->capture_buff, lexer->buff);
	}
	++lexer->location.column;
}

static inline bool lip_lexer_is_separator(char ch)
{
	return isspace(ch) || ch == ')' || ch == '(' || ch == ';' || ch == '"';
}

static inline lip_lex_status_t lip_lexer_lex_number(
	lip_lexer_t* lexer, lip_token_t* token
)
{
	char ch;
	while(lip_lexer_peek_char(lexer, &ch))
	{
		if(ch == '.' || isdigit(ch))
		{
			lip_lexer_consume_char(lexer);
			continue;
		}
		else if(!lip_lexer_is_separator(ch))
		{
			lip_lexer_consume_char(lexer);
			lip_lexer_make_token(
				lexer,
				token,
				LIP_TOKEN_NUMBER
			);
			return LIP_LEX_BAD_NUMBER;
		}
		else
		{
			break;
		}
	}

	return lip_lexer_make_token(lexer, token, LIP_TOKEN_NUMBER);
}

static inline lip_lex_status_t lip_lexer_lex_symbol(
	lip_lexer_t* lexer, lip_token_t* token
)
{
	char ch;
	while(lip_lexer_peek_char(lexer, &ch))
	{
		if(!lip_lexer_is_separator(ch))
		{
			lip_lexer_consume_char(lexer);
		}
		else
		{
			break;
		}
	}

	return lip_lexer_make_token(lexer, token, LIP_TOKEN_SYMBOL);
}

lip_lex_status_t lip_lexer_next_token(lip_lexer_t* lexer, lip_token_t* token)
{
	if(lexer->eos) { return LIP_LEX_EOS; }

	char ch;
	while(lip_lexer_peek_char(lexer, &ch))
	{
		token->start = lexer->location;
		lip_lexer_begin_capture(lexer);
		lip_lexer_consume_char(lexer);

		switch(ch)
		{
			case ' ':
			case '\t':
				lip_lexer_reset_capture(lexer);
				continue;
			case '\r':
				lip_lexer_reset_capture(lexer);
				++lexer->location.line;
				lexer->location.column = 1;

				if(lip_lexer_peek_char(lexer, &ch) && ch == '\n')
				{
					lip_lexer_consume_char(lexer);
				}
				continue;
			case '\n':
				lip_lexer_reset_capture(lexer);
				++lexer->location.line;
				lexer->location.column = 1;
				continue;
			case '(':
				return lip_lexer_make_token(lexer, token, LIP_TOKEN_LPAREN);
			case ')':
				return lip_lexer_make_token(lexer, token, LIP_TOKEN_RPAREN);
			case ';':
				while(lip_lexer_peek_char(lexer, &ch))
				{
					if(ch == '\r' || ch == '\n')
					{
						break;
					}
					else
					{
						lip_lexer_consume_char(lexer);
					}
				}
				continue;
			case '"':
				token->start = lexer->location;
				lip_lexer_reset_capture(lexer);
				lip_lexer_begin_capture(lexer);
				while(lip_lexer_peek_char(lexer, &ch))
				{
					// TODO: handle \"
					if(ch == '"')
					{
						lip_lexer_make_token(lexer, token, LIP_TOKEN_STRING);
						lip_lexer_consume_char(lexer);
						return LIP_LEX_OK;
					}
					else
					{
						lip_lexer_consume_char(lexer);
					}
				}
				lip_lexer_make_token(lexer, token, LIP_TOKEN_STRING);
				return LIP_LEX_BAD_STRING;
			case '-':
				lip_lexer_peek_char(lexer, &ch);
				if(isdigit(ch))
				{
					return lip_lexer_lex_number(lexer, token);
				}
				else if(lip_lexer_is_separator(ch))
				{
					return lip_lexer_make_token(lexer, token, LIP_TOKEN_SYMBOL);
				}
				else
				{
					return lip_lexer_lex_symbol(lexer, token);
				}
			default:
				if(isdigit(ch))
				{
					return lip_lexer_lex_number(lexer, token);
				}
				else
				{
					return lip_lexer_lex_symbol(lexer, token);
				}
		}
	}

	return LIP_LEX_EOS;
}

void lip_lexer_print_status(
	lip_write_fn_t write_fn, void* ctx,
	lip_lex_status_t status, lip_token_t* token
)
{
	lip_printf(write_fn, ctx, "%s", lip_lex_status_t_to_str(status));

	switch(status)
	{
		case LIP_LEX_OK:
			lip_printf(write_fn, ctx, " ");
			lip_token_print(write_fn, ctx, token);
		case LIP_LEX_EOS:
			break;
		case LIP_LEX_BAD_NUMBER:
		case LIP_LEX_BAD_STRING:
			lip_printf(write_fn, ctx,
				" '%.*s' %u:%u - %u:%u",
				(int)token->lexeme.length, token->lexeme.ptr,
				token->start.line, token->start.column,
				token->end.line, token->end.column
			);
			break;
	}
}
