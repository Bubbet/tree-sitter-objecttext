import XCTest
import SwiftTreeSitter
import TreeSitterObjecttext

final class TreeSitterObjecttextTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_objecttext())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Object Text grammar")
    }
}
