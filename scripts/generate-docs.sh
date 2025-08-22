#!/bin/bash

# Script to generate documentation locally for testing
# Usage: ./scripts/generate-docs.sh

set -e

echo "🔧 Reaction Framework - Documentation Generator"
echo "=============================================="

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "❌ Error: Doxygen is not installed!"
    echo "   Please install Doxygen:"
    echo "   - Ubuntu/Debian: sudo apt-get install doxygen graphviz"
    echo "   - macOS: brew install doxygen graphviz"
    echo "   - Windows: Download from https://www.doxygen.nl/download.html"
    exit 1
fi

# Check if Graphviz is installed (for diagrams)
if ! command -v dot &> /dev/null; then
    echo "⚠️  Warning: Graphviz (dot) is not installed!"
    echo "   Class diagrams and graphs will not be generated."
    echo "   Install with: sudo apt-get install graphviz (Ubuntu/Debian)"
else
    echo "✅ Graphviz found: $(dot -V 2>&1)"
fi

echo "✅ Doxygen found: $(doxygen --version)"

# Create output directory
echo "📁 Creating output directory..."
mkdir -p docs

# Clean previous documentation
if [ -d "docs/html" ]; then
    echo "🧹 Cleaning previous documentation..."
    rm -rf docs/html
fi

# Generate documentation
echo "📚 Generating documentation..."
if doxygen Doxyfile; then
    echo "✅ Documentation generated successfully!"
    
    # Check if HTML was generated
    if [ -d "docs/html" ] && [ -f "docs/html/index.html" ]; then
        echo "🌐 HTML documentation available at: docs/html/index.html"
        
        # Count generated files
        html_files=$(find docs/html -name "*.html" | wc -l)
        echo "📄 Generated $html_files HTML files"
        
        # Try to open in browser (optional)
        if command -v xdg-open &> /dev/null; then
            echo "🚀 Opening documentation in browser..."
            xdg-open "docs/html/index.html" 2>/dev/null || true
        elif command -v open &> /dev/null; then
            echo "🚀 Opening documentation in browser..."
            open "docs/html/index.html" 2>/dev/null || true
        else
            echo "💡 Open docs/html/index.html in your browser to view the documentation"
        fi
        
        echo ""
        echo "🎉 Documentation generation completed!"
        echo "   Main page: docs/html/index.html"
        echo "   Classes: docs/html/annotated.html"
        echo "   Files: docs/html/files.html"
        
    else
        echo "❌ Error: HTML documentation was not generated properly!"
        exit 1
    fi
else
    echo "❌ Error: Documentation generation failed!"
    echo "   Check the Doxyfile configuration and source code comments."
    exit 1
fi