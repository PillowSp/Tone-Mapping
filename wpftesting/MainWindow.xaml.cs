// ============================================================
// File: MainWindow.xaml.cs
// Author: Jakub Hanusiak
// Date: 5 sem, 2026-01-21
// Topic: Tone Mapping
//
// Description:
// This file contains the WPF GUI logic for loading images,
// applying HDR tone mapping using either OpenGL (GPU)
// or AVX2 Assembly (CPU), and displaying the results.
//
// ============================================================

using Microsoft.Win32;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

// ============================================================
// Native OpenGL-based tone mapping interface
// ============================================================
internal partial class ToneMapGL
{
    // --------------------------------------------------------
    // UploadToGL
    //
    // Description:
    // Uploads an HDR linear RGB image to OpenGL, performs
    // tone mapping in a fragment shader, and reads back
    // the final 8-bit BGRA image.
    //
    // Parameters:
    // linearRGB  - Input linear RGB float array [RGBRGB...]
    // width      - Image width in pixels (> 0)
    // height     - Image height in pixels (> 0)
    // outputBGRA - Output buffer for BGRA image (byte array)
    // exposure   - Exposure multiplier (> 0.0)
    // whitePoint - Reinhard white point (> 0.0)
    //
    // Output:
    // outputBGRA array is filled with tone-mapped image data
    // --------------------------------------------------------
    [DllImport("Clib.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void UploadToGL(
        float[] linearRGB,
        int width,
        int height,
        byte[] outputBGRA,
        float exposure,
        float whitePoint
    );

    // Initializes GLFW and OpenGL context
    [DllImport("Clib.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern bool InitGLFW();

    // Releases GLFW and OpenGL resources
    [DllImport("Clib.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void CleanupGLFW();
}

// ============================================================
// Native AVX2 Assembly-based tone mapping interface
// ============================================================
internal static class ToneMapAsm
{
    // --------------------------------------------------------
    // ToneMapAVX2
    //
    // Description:
    // Applies Extended Reinhard tone mapping using AVX2
    // instructions on a planar RGB float buffer.
    //
    // Parameters:
    // combined   - Planar float buffer [RRR...GGG...BBB...]
    // size       - Number of pixels (> 0)
    // exposure   - Exposure multiplier (> 0.0)
    // whitePoint - Reinhard white point (> 0.0)
    //
    // Output:
    // The combined buffer is modified in-place
    // --------------------------------------------------------
    [DllImport("ASMlib.dll", CallingConvention = CallingConvention.Cdecl)]
    internal static extern unsafe void ToneMapAVX2(
        float* combined,
        int size,
        float exposure,
        float whitePoint
    );
}

// ============================================================
// Main WPF application window
// ============================================================
namespace wpftesting
{
    public partial class MainWindow : Window
    {
        // ----------------------------------------------------
        // Private fields
        // ----------------------------------------------------

        // Original linear RGB image (HDR, linear space)
        private float[] _linearRGB;

        // Boosted version of the image (for HDR exaggeration)
        private float[] _boostedlinearRGB;

        // Loaded image bitmap (for display and metadata)
        private BitmapImage _bitmap;

        // ----------------------------------------------------
        // Constructor
        // ----------------------------------------------------
        public MainWindow()
        {
            InitializeComponent();
            Console.WriteLine(Environment.CurrentDirectory);
        }

        // ----------------------------------------------------
        // Window closing handler
        //
        // Ensures OpenGL resources are released properly
        // ----------------------------------------------------
        private void Window_Closing(object sender, CancelEventArgs e)
        {
            ToneMapGL.CleanupGLFW();
        }

        // ----------------------------------------------------
        // Clamp helper function
        //
        // Clamps a floating-point value to a given range
        // ----------------------------------------------------
        static float Clamp(float value, float min, float max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        // ----------------------------------------------------
        // BoostImage
        //
        // Description:
        // Multiplies the linear RGB image by a user-defined
        // HDR boost factor and updates the preview image.
        // ----------------------------------------------------
        private void BoostImage()
        {
            float hdrBoost;

            // Create a copy to preserve original data
            _boostedlinearRGB = _linearRGB.ToArray();

            if (float.TryParse(
                ColourBoostBox.Text,
                System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture,
                out hdrBoost))
            {
                // Enforce minimum boost
                if (hdrBoost < 1.0f)
                {
                    hdrBoost = 1.0f;
                    ColourBoostBox.Text = "1.0";
                }

                // Apply boost to all channels
                for (int i = 0; i < _boostedlinearRGB.Length; i++)
                {
                    _boostedlinearRGB[i] *= hdrBoost;
                }

                // Update preview image
                WriteableBitmap preview =
                    LinearRGBToBitmap(_boostedlinearRGB,
                    _bitmap.PixelWidth,
                    _bitmap.PixelHeight);

                PreviewImage.Source = preview;
            }
        }

        // ----------------------------------------------------
        // Colour boost value change handler
        // ----------------------------------------------------
        private void ColourBoost_ValueChanged(object sender, KeyEventArgs ek)
        {
            if (_linearRGB != null &&
                _bitmap != null &&
                ek.Key == Key.Enter)
            {
                BoostImage();
            }
        }

        // ----------------------------------------------------
        // InterleavedToPlanar
        //
        // Converts RGBRGBRGB... into three planar arrays
        // ----------------------------------------------------
        public static void InterleavedToPlanar(
            float[] rgb,
            int pixelCount,
            out float[] r,
            out float[] g,
            out float[] b)
        {
            r = new float[pixelCount];
            g = new float[pixelCount];
            b = new float[pixelCount];

            int src = 0;
            for (int i = 0; i < pixelCount; i++)
            {
                r[i] = rgb[src++];
                g[i] = rgb[src++];
                b[i] = rgb[src++];
            }
        }

        // ----------------------------------------------------
        // PlanarToInterleaved
        //
        // Converts [RRR...GGG...BBB...] into RGBRGBRGB...
        // ----------------------------------------------------
        public static float[] PlanarToInterleaved(float[] planar)
        {
            int n = planar.Length / 3;
            float[] interleaved = new float[planar.Length];

            for (int i = 0; i < n; i++)
            {
                interleaved[3 * i + 0] = planar[i];
                interleaved[3 * i + 1] = planar[n + i];
                interleaved[3 * i + 2] = planar[2 * n + i];
            }

            return interleaved;
        }

        // ----------------------------------------------------
        // LinearRGBToBitmap
        //
        // Converts linear RGB floats into an sRGB BGRA bitmap
        // ----------------------------------------------------
        static WriteableBitmap LinearRGBToBitmap(
            float[] linearRGB,
            int width,
            int height)
        {
            byte[] pixels = new byte[width * height * 4];

            for (int i = 0, j = 0; j < linearRGB.Length; i += 4, j += 3)
            {
                float r = Clamp(linearRGB[j + 0], 0f, 1f);
                float g = Clamp(linearRGB[j + 1], 0f, 1f);
                float b = Clamp(linearRGB[j + 2], 0f, 1f);

                // Apply gamma correction
                r = (float)Math.Pow(r, 1.0 / 2.2);
                g = (float)Math.Pow(g, 1.0 / 2.2);
                b = (float)Math.Pow(b, 1.0 / 2.2);

                pixels[i + 0] = (byte)(b * 255f);
                pixels[i + 1] = (byte)(g * 255f);
                pixels[i + 2] = (byte)(r * 255f);
                pixels[i + 3] = 255; // Alpha
            }

            WriteableBitmap wb = new WriteableBitmap(
                width, height, 96, 96, PixelFormats.Bgra32, null);

            wb.WritePixels(
                new Int32Rect(0, 0, width, height),
                pixels,
                width * 4,
                0);

            return wb;
        }

        // ----------------------------------------------------
        // GenerateOpenGL
        //
        // Description:
        // Performs HDR tone mapping using the GPU via OpenGL.
        // The linear RGB image is uploaded to OpenGL, processed
        // in a fragment shader, and read back as an 8-bit BGRA image.
        //
        // Output:
        // Displays the tone-mapped image and execution time.
        // ----------------------------------------------------
        private void GenerateOpenGL()
        {
            // Start GPU timing
            var swg = Stopwatch.StartNew();

            // Output buffer for BGRA pixels (8-bit per channel)
            byte[] outputByte =
                new byte[_bitmap.PixelWidth * _bitmap.PixelHeight * 4];

            // Read UI parameters
            float exposure = (float)ExposureSlider.Value;
            float whitePoint = (float)WhitePointSlider.Value;

            // Call native OpenGL tone mapping pipeline
            ToneMapGL.UploadToGL(
                _boostedlinearRGB,
                _bitmap.PixelWidth,
                _bitmap.PixelHeight,
                outputByte,
                exposure,
                whitePoint
            );

            // Create WPF bitmap for display
            WriteableBitmap output =
                new WriteableBitmap(
                    _bitmap.PixelWidth,
                    _bitmap.PixelHeight,
                    96,
                    96,
                    PixelFormats.Bgra32,
                    null);

            // Copy GPU result into WPF bitmap
            output.WritePixels(
                new Int32Rect(0, 0, _bitmap.PixelWidth, _bitmap.PixelHeight),
                outputByte,
                _bitmap.PixelWidth * 4,
                0
            );

            // Update UI
            OutputImage.Source = output;

            // Stop timing and display result
            swg.Stop();
            Console.WriteLine($"GPU OpenGL: {swg.ElapsedMilliseconds} ms");
            TimeValueLabel.Text = $"{swg.ElapsedMilliseconds} ms";
        }

        // ----------------------------------------------------
        // GenerateAsm
        //
        // Description:
        // Performs HDR tone mapping on the CPU using
        // hand-written AVX2 assembly code.
        // The linear RGB image is converted from interleaved
        // to planar format, processed in-place by the assembly
        // routine, then converted back for display.
        //
        // Output:
        // Displays the tone-mapped image and execution time.
        // ----------------------------------------------------
        private void GenerateAsm()
        {
            // Start CPU timing
            var sw = Stopwatch.StartNew();

            // Copy boosted linear RGB data to a temporary array
            float[] temp = _boostedlinearRGB.ToArray();
            float[] r, g, b;

            // Convert interleaved RGBRGB... layout
            // into separate planar R, G, B arrays
            InterleavedToPlanar(
                temp,
                _bitmap.PixelWidth * _bitmap.PixelHeight,
                out r, out g, out b);

            int n = r.Length;

            // Combine planar channels into a single buffer:
            // [ R... | G... | B... ]
            float[] combined = new float[n * 3];
            Array.Copy(r, 0, combined, 0, n);
            Array.Copy(g, 0, combined, n, n);
            Array.Copy(b, 0, combined, 2 * n, n);

            // Read UI parameters
            float exposure = (float)ExposureSlider.Value;
            float whitePoint = (float)WhitePointSlider.Value;

            // Call AVX2 assembly tone mapping routine
            unsafe
            {
                fixed (float* ptr = combined)
                {
                    ToneMapAsm.ToneMapAVX2(ptr, n, exposure, whitePoint);
                }
            }

            // Convert planar buffer back to interleaved format
            temp = PlanarToInterleaved(combined);

            // Create output bitmap and display result
            OutputImage.Source = LinearRGBToBitmap(
                temp,
                _bitmap.PixelWidth,
                _bitmap.PixelHeight);

            // Stop timing and display result
            sw.Stop();
            TimeValueLabel.Text = $"{sw.ElapsedMilliseconds} ms";
        }

        // ----------------------------------------------------
        // LoadImage_Click
        //
        // Description:
        // Loads an image from disk, converts it from sRGB
        // to linear RGB floating-point format, and initializes
        // internal buffers used for tone mapping.
        //
        // Output:
        // Displays the original image and prepares HDR data.
        // ----------------------------------------------------
        private void LoadImage_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog
            {
                Filter = "Image Files|*.jpg;*.jpeg;*.png;*.bmp"
            };

            if (dialog.ShowDialog() == true)
            {
                // Load image into WPF bitmap
                BitmapImage bitmap = new BitmapImage();
                bitmap.BeginInit();
                bitmap.UriSource = new Uri(dialog.FileName);
                bitmap.CacheOption = BitmapCacheOption.OnLoad;
                bitmap.EndInit();

                // Display original image
                OriginalImage.Source = bitmap;
                _bitmap = bitmap;

                // Convert bitmap to writable format
                WriteableBitmap wb = new WriteableBitmap(bitmap);

                int width = wb.PixelWidth;
                int height = wb.PixelHeight;

                // BGRA pixel buffer
                byte[] pixels = new byte[width * height * 4];
                wb.CopyPixels(pixels, width * 4, 0);

                // Linear RGB float buffer (HDR)
                float[] linearRGB = new float[width * height * 3];

                // Convert from sRGB to linear RGB
                for (int i = 0, j = 0; i < pixels.Length; i += 4, j += 3)
                {
                    float b = pixels[i + 0] / 255f;
                    float g = pixels[i + 1] / 255f;
                    float r = pixels[i + 2] / 255f;

                    // Inverse gamma correction (sRGB → linear)
                    r = (float)Math.Pow(r, 2.2f);
                    g = (float)Math.Pow(g, 2.2f);
                    b = (float)Math.Pow(b, 2.2f);

                    linearRGB[j + 0] = r;
                    linearRGB[j + 1] = g;
                    linearRGB[j + 2] = b;
                }

                // Store linear HDR image
                _linearRGB = linearRGB;

                // Apply initial HDR boost and preview
                BoostImage();
            }
        }

        // ----------------------------------------------------
        // Generate button handler
        // ----------------------------------------------------
        private void Generate_Click(object sender, RoutedEventArgs e)
        {
            if (_linearRGB == null)
                return;

            if (OpenGLRadio.IsChecked == true)
                GenerateOpenGL();
            else
                GenerateAsm();
        }
    }
}
