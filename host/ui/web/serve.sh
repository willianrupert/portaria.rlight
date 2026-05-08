#!/bin/bash

# Servidor local para preview da UI do rlight
# Simula a tela de 7" (600x1024) do Orange Pi

PORT=8080
DIR="$(cd "$(dirname "$0")" && pwd)"

echo "🚀 Iniciando servidor da UI rlight..."
echo "📱 Preview da tela 7\" disponível em:"
echo "   http://localhost:$PORT/preview.html"
echo ""
echo "💡 Dicas:"
echo "   - Use o botão 'Recarregar UI' para ver alterações"
echo "   - Use 'Alternar Escala' se a tela não couber no seu monitor"
echo "   - Pressione Cmd+R para recarregar rapidamente"
echo ""
echo "⚠️  Pressione Ctrl+C para parar o servidor"
echo ""

cd "$DIR"
python3 -m http.server $PORT