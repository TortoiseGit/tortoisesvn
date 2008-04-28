using System;
using System.Runtime.InteropServices;

namespace Interop.BugTraqProvider
{
    [ComVisible(true), InterfaceType(ComInterfaceType.InterfaceIsDual), Guid("298B927B-7220-423C-B7B4-6E241F00CD93")]
    public interface IBugTraqProvider
    {
        [return: MarshalAs(UnmanagedType.VariantBool)]
        bool ValidateParameters(IntPtr hParentWnd, [MarshalAs(UnmanagedType.BStr)] string parameters);

        [return: MarshalAs(UnmanagedType.BStr)]
        string GetLinkText(IntPtr hParentWnd, [MarshalAs(UnmanagedType.BStr)] string parameters);

        [return: MarshalAs(UnmanagedType.BStr)]
        string GetCommitMessage(IntPtr hParentWnd, [MarshalAs(UnmanagedType.BStr)] string parameters,
                                [MarshalAs(UnmanagedType.BStr)] string originalMessage);
    }
}
