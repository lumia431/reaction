#!/bin/bash

# Script to validate Doxygen configuration without actually generating docs
# Usage: ./scripts/validate-config.sh

echo "🔧 Reaction Framework - Configuration Validator"
echo "==============================================="

# Check if Doxyfile exists
if [ ! -f "Doxyfile" ]; then
    echo "❌ Error: Doxyfile not found!"
    exit 1
fi

echo "✅ Doxyfile found"

# Check input directories
echo "📁 Checking input directories..."

INPUT_DIRS=("include" "example" "test")
for dir in "${INPUT_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        file_count=$(find "$dir" -name "*.h" -o -name "*.hpp" -o -name "*.cpp" | wc -l)
        echo "  ✅ $dir/ ($file_count files)"
    else
        echo "  ❌ $dir/ not found"
    fi
done

# Check README.md
if [ -f "README.md" ]; then
    echo "✅ README.md found"
else
    echo "❌ README.md not found"
fi

# Check for documented classes in key files
echo "📖 Checking documentation in key files..."

KEY_FILES=(
    "include/reaction/reaction.h"
    "include/reaction/react.h"
    "include/reaction/expression.h"
    "include/reaction/observerNode.h"
)

total_comments=0
for file in "${KEY_FILES[@]}"; do
    if [ -f "$file" ]; then
        # Count Doxygen-style comments (/** or ///)
        comment_count=$(grep -c '^\s*\(/\*\*\|\*\s\|///\)' "$file" || true)
        total_comments=$((total_comments + comment_count))
        echo "  📄 $file: $comment_count comment lines"
    else
        echo "  ❌ $file not found"
    fi
done

echo "📊 Total documentation lines: $total_comments"

# Validate key Doxyfile settings
echo "⚙️  Validating Doxyfile settings..."

check_setting() {
    local setting=$1
    local expected=$2
    local actual=$(grep "^$setting\s*=" Doxyfile | head -1 | cut -d'=' -f2 | xargs)
    
    if [ "$actual" = "$expected" ]; then
        echo "  ✅ $setting = $actual"
    else
        echo "  ⚠️  $setting = $actual (expected: $expected)"
    fi
}

check_setting "GENERATE_HTML" "YES"
check_setting "SOURCE_BROWSER" "YES"
check_setting "GENERATE_TREEVIEW" "YES"
check_setting "CLASS_GRAPH" "YES"
check_setting "RECURSIVE" "YES"

# Check output directory setting
output_dir=$(grep "^OUTPUT_DIRECTORY" Doxyfile | cut -d'=' -f2 | xargs)
echo "  📁 Output directory: $output_dir"

echo ""
echo "🎉 Configuration validation completed!"
echo "   Ready for documentation generation"
echo "   Run 'doxygen Doxyfile' when Doxygen is available"