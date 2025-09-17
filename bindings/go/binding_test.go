package tree_sitter_objecttext_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_objecttext "github.com/tree-sitter/tree-sitter-objecttext/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_objecttext.Language())
	if language == nil {
		t.Errorf("Error loading Object Text grammar")
	}
}
