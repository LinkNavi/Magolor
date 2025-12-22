#!/bin/bash
# Install Magolor LSP configuration for Neovim

set -e

NVIM_CONFIG="$HOME/.config/nvim"
MAGOLOR_COMPILER="$HOME/Programming/Magolor/MagolorCompiler"

echo "╔════════════════════════════════════════╗"
echo "║  Magolor LSP Setup for Neovim         ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Check if Neovim config exists
if [ ! -d "$NVIM_CONFIG" ]; then
    echo "❌ Neovim config directory not found at $NVIM_CONFIG"
    exit 1
fi

echo "✓ Found Neovim config at $NVIM_CONFIG"

# Check if Magolor compiler is built
if [ ! -f "$MAGOLOR_COMPILER/target/debug/Magolor" ]; then
    echo "❌ Magolor compiler not found. Building..."
    cd "$MAGOLOR_COMPILER"
    Zora build
fi

echo "✓ Magolor compiler found"

# Install Magolor binary
if ! command -v magolor &> /dev/null; then
    echo "Installing magolor binary..."
    sudo cp "$MAGOLOR_COMPILER/target/debug/Magolor" /usr/local/bin/magolor
    sudo chmod +x /usr/local/bin/magolor
fi

echo "✓ Magolor binary installed"

# Create necessary directories
mkdir -p "$NVIM_CONFIG/lua/plugins"
mkdir -p "$NVIM_CONFIG/ftdetect"
mkdir -p "$NVIM_CONFIG/after/syntax"

# Copy LSP configuration
echo "Installing LSP configuration..."
cat > "$NVIM_CONFIG/lua/plugins/magolor-lsp.lua" << 'EOF'
-- Magolor LSP Configuration
return {
  {
    "neovim/nvim-lspconfig",
    dependencies = {
      "williamboman/mason.nvim",
      "williamboman/mason-lspconfig.nvim",
    },
    config = function()
      local lspconfig = require("lspconfig")
      local configs = require("lspconfig.configs")

      if not configs.magolor_lsp then
        configs.magolor_lsp = {
          default_config = {
            cmd = { "magolor", "lsp" },
            filetypes = { "magolor" },
            root_dir = function(fname)
              return lspconfig.util.root_pattern("project.toml", ".git")(fname)
                or lspconfig.util.path.dirname(fname)
            end,
            settings = {},
          },
        }
      end

      local capabilities = vim.lsp.protocol.make_client_capabilities()
      local has_cmp, cmp_nvim_lsp = pcall(require, "cmp_nvim_lsp")
      if has_cmp then
        capabilities = cmp_nvim_lsp.default_capabilities(capabilities)
      end

      local on_attach = function(client, bufnr)
        local opts = { buffer = bufnr, silent = true }
        vim.keymap.set("n", "gd", vim.lsp.buf.definition, opts)
        vim.keymap.set("n", "K", vim.lsp.buf.hover, opts)
        vim.keymap.set("n", "gr", vim.lsp.buf.references, opts)
        vim.keymap.set("n", "<leader>rn", vim.lsp.buf.rename, opts)
        vim.keymap.set("n", "<leader>ca", vim.lsp.buf.code_action, opts)
        vim.keymap.set("n", "<leader>f", function()
          vim.lsp.buf.format({ async = true })
        end, opts)
      end

      lspconfig.magolor_lsp.setup({
        on_attach = on_attach,
        capabilities = capabilities,
      })

      vim.diagnostic.config({
        virtual_text = true,
        signs = true,
        underline = true,
        update_in_insert = false,
        severity_sort = true,
      })
    end,
  },
}
EOF

echo "✓ LSP configuration installed"

# Copy filetype detection
echo "Installing filetype detection..."
cat > "$NVIM_CONFIG/ftdetect/magolor.lua" << 'EOF'
vim.filetype.add({
  extension = {
    mg = "magolor",
  },
})
EOF

echo "✓ Filetype detection installed"

# Copy syntax highlighting
echo "Installing syntax highlighting..."
cat > "$NVIM_CONFIG/after/syntax/magolor.vim" << 'EOF'
if exists("b:current_syntax")
  finish
endif

syn keyword magolorKeyword fn let return if else while for match class new this using pub priv static mut cimport
syn keyword magolorType int float string bool void
syn keyword magolorBoolean true false
syn keyword magolorConstant None Some

syn region magolorComment start="//" end="$"
syn region magolorString start='"' end='"' skip='\\"'
syn region magolorInterpolatedString start='\$"' end='"'

syn match magolorNumber /\<\d\+\>/
syn match magolorFloat /\<\d\+\.\d\+\>/
syn match magolorFunction /\w\+\s*(/me=e-1

hi def link magolorKeyword Keyword
hi def link magolorType Type
hi def link magolorBoolean Boolean
hi def link magolorConstant Constant
hi def link magolorComment Comment
hi def link magolorString String
hi def link magolorInterpolatedString String
hi def link magolorNumber Number
hi def link magolorFloat Float
hi def link magolorFunction Function

let b:current_syntax = "magolor"
EOF

echo "✓ Syntax highlighting installed"

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  Installation Complete! 🎉             ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "Restart Neovim and open a .mg file to test the LSP."
echo ""
echo "LSP Keybindings:"
echo "  gd          - Go to definition"
echo "  K           - Show hover information"
echo "  gr          - Show references"
echo "  <leader>rn  - Rename symbol"
echo "  <leader>ca  - Code actions"
echo "  <leader>f   - Format code"
echo ""
