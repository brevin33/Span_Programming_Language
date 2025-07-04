package tree_sitter_span_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_span "github.com/brevin33/span_programming_language/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_span.Language())
	if language == nil {
		t.Errorf("Error loading Span grammar")
	}
}
