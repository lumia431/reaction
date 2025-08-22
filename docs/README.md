# Reaction Framework Documentation

This directory contains the documentation generation system for the Reaction Framework.

## 📚 Documentation Overview

The documentation is automatically generated using [Doxygen](https://www.doxygen.nl/) and deployed to GitHub Pages. It includes:

- **API Documentation**: Complete reference for all classes, functions, and types
- **Class Diagrams**: Visual representation of class relationships  
- **Dependency Graphs**: Shows how components interact
- **Usage Examples**: Code examples from the `example/` directory
- **Source Code Browser**: Browse the source code with syntax highlighting

## 🚀 Accessing the Documentation

### Online Documentation
The latest documentation is automatically deployed to GitHub Pages:
- **Live Documentation**: https://lumia431.github.io/reaction/

### Local Generation
You can generate documentation locally for development:

```bash
# Using the provided script (recommended)
./scripts/generate-docs.sh

# Or manually using Doxygen
doxygen Doxyfile
```

## 🔄 Automatic Updates

The documentation is automatically updated when:

1. **Push to main/master**: Documentation is generated and deployed to GitHub Pages
2. **Pull Request**: Documentation preview is generated as an artifact
3. **Manual Trigger**: You can manually trigger the workflow from GitHub Actions

## 📋 Configuration

The documentation system consists of:

- `Doxyfile`: Main Doxygen configuration file
- `.github/workflows/documentation.yml`: GitHub Actions workflow
- `scripts/generate-docs.sh`: Local generation script
- `docs/`: Output directory (ignored by git)

### Key Configuration Options

| Option | Value | Description |
|--------|-------|-------------|
| `PROJECT_NAME` | "Reaction Framework" | Project title |
| `OUTPUT_DIRECTORY` | `docs/` | Where to generate files |
| `INPUT` | `README.md include/ example/ test/` | Source directories |
| `RECURSIVE` | `YES` | Process subdirectories |
| `GENERATE_HTML` | `YES` | Generate HTML output |
| `SOURCE_BROWSER` | `YES` | Include source code browser |
| `GENERATE_TREEVIEW` | `YES` | Generate navigation tree |
| `CLASS_GRAPH` | `YES` | Generate class diagrams |

## 🛠️ Development

### Prerequisites

To generate documentation locally, you need:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows
# Download from https://www.doxygen.nl/download.html
```

### Adding Documentation

To improve the documentation:

1. **Add Doxygen comments** to your code:
   ```cpp
   /**
    * @brief Brief description of the function
    * 
    * Detailed description with more information about what
    * the function does and how to use it.
    * 
    * @param param1 Description of parameter 1
    * @param param2 Description of parameter 2
    * @return Description of return value
    * @throws std::exception When this exception is thrown
    * 
    * @code
    * // Usage example
    * auto result = myFunction(arg1, arg2);
    * @endcode
    */
   ReturnType myFunction(Type1 param1, Type2 param2);
   ```

2. **Use Doxygen special commands**:
   - `@brief`: Short description
   - `@param`: Parameter description
   - `@return`: Return value description
   - `@throws`: Exception description
   - `@code/@endcode`: Code examples
   - `@note`: Important notes
   - `@warning`: Warnings
   - `@see`: Cross-references

3. **Test locally** before committing:
   ```bash
   ./scripts/generate-docs.sh
   ```

### Workflow Details

The GitHub Actions workflow:

1. **Checkout**: Gets the latest code
2. **Install**: Installs Doxygen and Graphviz
3. **Generate**: Runs Doxygen to create documentation
4. **Deploy**: Publishes to GitHub Pages (main branch only)
5. **Preview**: Creates artifacts for PR previews

## 🎯 Best Practices

1. **Write clear comments**: Use complete sentences and proper grammar
2. **Include examples**: Show how to use functions and classes
3. **Document parameters**: Explain what each parameter does
4. **Mention exceptions**: Document what exceptions can be thrown
5. **Use cross-references**: Link related functions and classes
6. **Keep it updated**: Update documentation when code changes

## 🔧 Troubleshooting

### Common Issues

**Documentation not generating:**
- Check Doxygen syntax in comments
- Verify file paths in Doxyfile
- Look for errors in GitHub Actions logs

**Missing diagrams:**
- Ensure Graphviz is installed
- Check `HAVE_DOT = YES` in Doxyfile
- Verify class relationships are documented

**GitHub Pages not updating:**
- Check repository settings → Pages
- Ensure workflow has proper permissions
- Verify the workflow completed successfully

### Getting Help

- Check the [Doxygen manual](https://www.doxygen.nl/manual/)
- Look at existing documented code for examples
- Review GitHub Actions logs for error messages