import XCTest
import SwiftTreeSitter
import TreeSitterSpan

final class TreeSitterSpanTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_span())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Span grammar")
    }
}
