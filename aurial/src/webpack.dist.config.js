var path = require('path');
var webpack = require('webpack');
var HtmlWebpackPlugin = require('html-webpack-plugin');
var CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = {
  target: "web",
  entry: path.resolve(__dirname, 'js'),
  output: {
    path: path.resolve(__dirname, '../../build', 'htdocs'),
    filename: 'index.js'
  },
  plugins: [
    new webpack.DefinePlugin({
      'process.env': {
        NODE_ENV: JSON.stringify('production')
      }
    }),
    /*
    new webpack.optimize.UglifyJsPlugin({
      compress: {
        warnings: false,
        dead_code: true
      }
    }),
    */
    new HtmlWebpackPlugin({
      title: 'Aurmpd',
      template: 'src/index.html'
    }),
    new CopyWebpackPlugin([
      {from: 'src/css', to: 'css'},
      {from: 'src/js/jquery-2.2.1.min.js', to: 'js'},
      {from: 'README.md'}
    ])
  ],
  module: {
    rules: [
      {
        test: /\.js$/,
        loader: 'babel-loader',
        exclude: /node_modules/,
        options: {
          "plugins": [
            ["transform-react-jsx", { "pragma": "h" }]
          ],
          "presets": ["es2015", "stage-0"]
        }
      }
    ]
  }
};
